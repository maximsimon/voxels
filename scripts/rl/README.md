# 1. Big picture

There are two halves. The **learning half** lives in `scripts/rl/` (my 8 files). The **simulation half** already existed in `scripts/vector_env/vec_env.py` and the C++ core. The only file that knows about both is `rl_main.py`.

```text
                        rl_main.py        ← wiring only; every number comes from rl_config.py, so nothing here can shadow it
                              │
        ┌─────────────────────┼──────────────────────┐
        ↓                     ↓                      ↓
   rl_config.py          trainer.py ──────────►  vec_env.VecEnv   (scripts/vector_env, not mine)
   (numbers only)      THE TRAINING LOOP              │
        │                     │                        ├─ process 0 ─► voxel_sim ─► raylib window
        │                     │                        ├─ process 1 ─► voxel_sim ─► raylib window
        │                     │                        └─ process N ─► voxel_sim ─► raylib window
        │                     │
        │                     ├─► obs_features.py   raw obs dict ──► (uint8 pixels, float32 states)
        │                     ├─► policy.py         images + states ──► actions [N,6]
        │                     ├─► reward.py         (obs, actions) ──► reward [N]   ← called *inside* env.step
        │                     ├─► buffer.py         stores one rollout, computes returns
        │                     └─► algorithm.py      consumes minibatches, returns metrics
        │
        └─► RLConfig handed to the trainer as one object
```

The loop that ties them together, in one line:

```text
observation dict
   → obs_features.ObsFeatures        → images [N,H,W,C] uint8  +  states [N,state_dim] float32
   → policy.act(images, states, step)→ [N,6] action batch
   → vec_env.step(actions)           → N processes step the sim concurrently
        └─ inside step: reward.reward_fn(obs, actions) → [N] reward
   → trainer.collect_rollout         → buffer.add(images, states, ...)  ×64 times
   → buffer.compute_returns
   → algorithm.update(buffer)        → metrics dict → printed
```

---

# 2. Each file

### `rl_main.py` — entry point and assembler of choices
- **For:** starts the process, decides *which* policy/reward/algorithm run, and holds every number of a run.
- **Why:** there is one place to look to change what a run *is*, rather than hunting through modules.
- **Provides:** `POLICY` / `REWARD` / `ALGORITHM` (the wiring block, lines 29–33) and `main()`. It builds the config with a bare `RLConfig()`, so every number comes from `rl_config.py` and cannot be shadowed from here.
- **Used by:** you. Nothing imports it.
- **Fits:** the very top and bottom of the pipeline. It inserts `build/` and `scripts/vector_env` into `sys.path` (lines 17–18) because `voxel_sim.so` and `vec_env.py` live elsewhere.

### `rl_config.py` — the numbers, nothing else
- **For:** three dataclasses holding settings. `EnvConfig.to_sim_config()` translates them into the keyword dict `voxel_sim.Configure()` wants.
- **Why:** so no module contains a hardcoded 64 or 200. Note what is *absent*: no policy/reward names — those are objects, not strings, and live in `rl_main.py`.
- **Provides:** `EnvConfig`, `TrainConfig`, `RLConfig`, and the two key tuples `IMAGE_KEYS` / `STATE_KEYS` (which observation keys reach the learner).
- **Used by:** `rl_main.py` (constructs it with `RLConfig()`), `trainer.py` (reads it), and `EnvConfig.to_sim_config()` is passed to `VecEnv`.
- **Fits:** passive. It is the parameter object of the whole run.

### `obs_features.py` — the boundary between simulator and learner
- **For:** `ObsFeatures.__call__(obs)` turns the stacked observation **dict** (`lidar_scan`, `camera_front`, `position`, `yaw`, `angular_vel`, …) into **two** arrays, because they want opposite things: `images` as `uint8 [num_envs, H, W, C]` with the spatial layout intact, and `states` as a flat `float32 [num_envs, state_dim]`. With the shipped config that is 128×80×3 = 30720 pixels plus 186 floats (180 lidar + 3 angular + 3 linear vel).
- **Why:** this is the single most important decoupling in the setup. Everything downstream works on fixed-shape tensors and never learns the simulator's dict layout. The split also decides the *cost* of a rollout: pixels stay `uint8` all the way into the buffer, so a 64×4 rollout is ~8 MB instead of ~32 MB for a float32 copy, and normalisation is left to the network where it belongs.
- **Provides:** the `ObsFeatures` callable, plus `.image_shape`, `.state_dim` and `.layout` (which key contributed how many values).
- **Used by:** `trainer.py` only.
- **Fits:** the first step of every training iteration. Both layouts are *frozen by the first observation* (`obs_features.py:44-47` for pixels, `:58-61` for states), which is what guarantees the buffer's preallocated shape stays valid for the whole run.

### `policy.py` — the network file (the network doesn't exist yet)
- **For:** turns observations into actions. `Policy.act(images, states, step) → [num_envs, 6]`. It receives the **two branches** from `obs_features` — `images` as `uint8 [N,H,W,C]`, `states` as `float32 [N,state_dim]` — never the raw dict. The action layout is `[vx, vy, vz, wx, wy, wz]` — three linear velocities, three angular velocities; the simulator only acts on `vx`, `vz` (forward, strafe) and `wy` (yaw rate).
- **Why:** the learner must never be the thing that decides how to drive. Right now `RandomPolicy` emits uniform noise and `ConstantPolicy` emits a fixed command. **When you add your network, this is the file it goes in** — subclass `Policy`, implement `act`, and the rest of the setup is untouched. Keeping pixels `uint8` here is deliberate: the first conv layer is where `/255` belongs.
- **Provides:** `Policy` base (`act`, the `on_rollout_start` hook for recurrent state, `_check` shape validation), `RandomPolicy`, `ConstantPolicy`.
- **Used by:** `trainer.py`, which constructs the class and calls `act()` once per step.
- **Fits:** inside the rollout loop, between features and `env.step`. Whether it can see the camera image at all is decided by `IMAGE_KEYS` in `rl_config.py`.

### `reward.py` — what a frame is worth
- **For:** plain functions with the signature `(obs, actions) → np.ndarray[N]`. It sees the **raw observation dict**, not the feature vector, so it can use anything the simulator reports.
- **Why:** the reward is a task definition, not a mechanism. Two placeholders: `forward_velocity_reward` (pay for forward velocity, optionally dock for spinning) and `blocked_velocity_reward` (penalty when commanded forward but not moving — a crude wall hit).
- **Provides:** `forward_velocity_reward`, `blocked_velocity_reward`, and the tuning constants above them.
- **Used by:** `trainer.py` — but only to *install* it: `env.reward_fn = self.reward_fn` (`trainer.py:35`). After that the reward is called by `VecEnv.step()` on the sim side.
- **Fits:** invisibly inside `env.step`. The trainer never computes a reward itself; it just reads the number that comes back. That's why adding a new reward needs zero changes to the loop.

### `buffer.py` — the hand-off point (least obvious file)
- **For:** `RolloutBuffer` stores **one rollout** — a fixed `[rollout_steps, num_envs]` block of preallocated arrays — plus three operations on it.
- **Why:** the collector and the learner need different things. Collection is sequential in time and cheap; learning wants flat, shuffled, *statistically independent* samples. The buffer is the translation layer, and it lets one expensive rollout be reused for several gradient steps.
- **Provides:**
  - `add(images, states, actions, rewards, dones)` — appends one vectorized frame for all N envs. The two observation branches are stored **separately**, and pixels must stay `uint8` (`buffer.py:72-73` raises otherwise) so a rollout costs ~8 MB instead of ~32 MB.
  - `compute_returns()` — walks the rollout **backwards** to build discounted returns, zeroing the running return at each episode boundary (`buffer.py:95-101`)
  - `batches(batch_size)` — flattens `[64, 4, …]` into 256 rows (`_iter_batches`, `buffer.py:112`), shuffles them once, yields minibatches of dicts keyed `images` / `states` / `actions` / `rewards` / `returns` / `dones`
  - `summary()` — mean reward / return / done rate, purely for logging
- **Used by:** `trainer.py` fills it; `algorithm.py` reads it. The algorithm never touches the simulator.
- **Fits:** between collection and learning. One instance lives on the trainer and is `clear()`ed at the top of each rollout.

### `algorithm.py` — the learner
- **For:** the only component allowed to *change* anything. `Algorithm.update(buffer)` iterates the buffer's minibatches, calls `_update_batch` on each, and averages the returned scalars.
- **Why:** so the learning rule is isolated from data collection. Swapping REINFORCE for PPO means writing one class here and changing one line in `rl_main.py`.
- **Provides:** `Algorithm` base (`update`, abstract `_update_batch`, `batch_size`), and `PlaceholderAlgorithm` which deliberately does **no learning** — it just reports batch statistics, which is what proves the collect → compute → update → log pipeline is wired correctly.
- **Used by:** `trainer.py` calls `update()` once per iteration (`trainer.py:151`).
- **Fits:** the end of the pipeline. Everything about it is batch-shaped: it never sees an environment, an observation key, or a reward formula.

### `trainer.py` — the training loop and the assembler
- **For:** `Trainer.run()` is the `for iteration in range(...)` loop. It also does the plumbing no other module should know about:
  - `setup()` — installs the reward, loads the world pool, resets the envs, freezes the feature layout, and **builds the policy and the algorithm** (`trainer.py:68-74`)
  - `_spawn_poses()` — random start poses so N envs don't begin as N identical copies
  - `collect_rollout()` — the 64-step data-gathering loop, with episode bookkeeping
  - `_splice()` — after a single env is reset mid-rollout, the stacked observation still holds that env's last frame; this overwrites that row with the fresh observation
- **Why it's the assembler:** `image_shape` and `state_dim` are not known until the first reset has run and the layout has frozen. Someone has to wire the objects together, and it's the one component that already holds the env.
- **Used by:** `rl_main.py`.
- **Fits:** the center. It knows *when* to call each other module, and knows nothing about *how* any of them work.

---

# 3. One training iteration, with the responsible file at each step

Config as shipped: 4 envs, `rollout_steps=64`, `batch_size=64`, `max_episode_steps=200`.

```text
Trainer.run()                                     [trainer.py:131]
 │
 ├─► setup()                                     [trainer.py:52]   once per run
 │     ├─ env.reward_fn = reward_fn               [trainer.py:35] → [reward.py]
 │     ├─ env.load_worlds(world_folder)           [trainer.py:57] → [vec_env:354]
 │     ├─ env.reset(worlds, spawn_poses)          [trainer.py:61] → [vec_env.VecEnv]
 │     │     └─► N processes call voxel_sim.Reset() → observation dict per env
 │     ├─ features(obs)  → freezes layout         [trainer.py:63] → [obs_features.py:27]
 │     ├─ build policy                            [trainer.py:68] ← from POLICY in rl_main
 │     ├─ build algorithm                         [trainer.py:69]
 │     └─ build RolloutBuffer                     [trainer.py:73] ← 64×4 preallocated
 │
 └─► loop iteration 1..20:
       │
       └─► collect_rollout()                      [trainer.py:93]
             │
             ├─ buffer.clear()                    [trainer.py:94] → [buffer.py:55]
             ├─ policy.on_rollout_start()         [trainer.py:95] → [policy.py:27]
             │
             └─ for step in 0..63:
                  │
                  ├─ images, states = features(obs)         [trainer.py:102]  uint8 + float32
                  ├─ actions  = policy.act(images, states, step) [trainer.py:103] → [policy.py:23]  [4,6]
                  ├─ obs, rew, term, trunc, info = env.step(actions)  [trainer.py:104] → [vec_env:295]
                  │       └─ inside step, per frame:
                  │            reward = env.reward_fn(obs, actions)  [vec_env:308] → [reward.py]  [4]
                  │            └─► N processes call voxel_sim.Step() concurrently
                  │
                  ├─ finished = terminated | truncated | (episode_len >= 200)  [trainer.py:109]
                  ├─ buffer.add(images, states, actions, rew, finished)   [trainer.py:112] → [buffer.py:60]
                  ├─ for each finished env:
                  │     env.reset_many(...)                   [trainer.py:121] → [vec_env:329]
                  │     _splice(obs, idx, single)             [trainer.py:124] → [trainer.py:45]
                  └─ obs = new observation  ───────────── loop back
       │
       ├─► buffer.compute_returns()                [trainer.py:149] → [buffer.py:91]   backwards, gamma=0.99
       ├─► algorithm.update(buffer)                [trainer.py:151] → [algorithm.py:24]
       │     └─ for batch in buffer.batches(64):   [algorithm.py:27] → [buffer.py:104]  256 rows → 4 minibatches
       │           _update_batch(batch)            [algorithm.py:28] → [algorithm.py:56] → metrics
       ├─► buffer.summary()                        [trainer.py:152] → [buffer.py:136]  for logging
       └─► print one line                          [trainer.py:160]
```

Per iteration: **256 transitions collected** (64 × 4), **4 minibatches of 64** consumed by the algorithm.

---

# 4. The concepts, in terms of this code

**What the buffer actually stores.** Seven parallel arrays shaped `[64, 4, …]`: `images` `[64,4,H,W,C]` uint8, `states` `[64,4,state_dim]`, `actions` `[64,4,6]`, `rewards` `[64,4]`, `dones` `[64,4]`, `returns` `[64,4]`, plus a seed for shuffling. One row-slice `t` is *one moment in time across all four robots* — that's why `add()` takes whole `[4, …]` branches, not one transition. The two observation branches are kept apart because they want opposite dtypes: pixels stay `uint8` so a rollout costs ~8 MB instead of ~32 MB, and normalisation is left to the network. `dones` exists so `compute_returns()` knows where one episode's future stops contaminating the next episode's return. The alternative to storing the rollout at all is to throw the data away after one update, which is what single-env online methods do; the buffer is what lets several update steps reuse the same expensive rollout.

**What the algorithm does.** Today: nothing but arithmetic on the batch — `batch_return_mean`, `advantage_mean`, `image_mean`, `loss=0.0`. Tomorrow: a loss function plus a gradient step. Its contract is fixed: batches in, a flat `dict[str, float]` out. The trainer uses that dict only for logging, so you can return whatever you want (`entropy`, `kl`, `value_loss`) and nothing breaks. **One seam to be aware of:** `PlaceholderAlgorithm` is constructed as `(image_shape, state_dim, action_dim, num_envs, seed, batch_size)` — i.e. only *shapes* — but *not* the policy, because it has nothing to update. When you write a real learner you must also hand it the network: either pass the policy object into `algorithm_cls(...)` at `trainer.py:69-70`, or have the algorithm own the parameters and the policy read them. That's the one place the architecture will need to grow.

**What the policy / network does.** It is the only component allowed to convert an observation into an action, and it is the only one that will hold learned weights. It receives two branches for all four robots at once — `images` `uint8 [4,H,W,C]` and `states` `float32 [4,state_dim]` — and must return `[4, 6]`. `_check()` enforces that shape so a network bug surfaces as a clear error instead of a crash inside the C++ `Step()`.

**What the training loop does.** Owns *when*, never *how*: reset → collect 64 steps → compute returns → update → log, repeated. It also owns the three bookkeeping concerns that would otherwise be scattered: episode-length counters for the time limit, spawning N robots at different poses, and splicing fresh observations back into the stacked batch after a mid-rollout reset.

**Why the loop can't just call the environment and the network directly.** Four reasons, all visible in this repo:
1. **Shape.** With `VecEnv`, observations arrive already stacked as `[4, …]`. The loop is inherently *vectorized over environments* — policies act on 4 rows at once, buffers store 4 columns at once. Writing this as a single-env loop throws away the parallelism the VecEnv class exists to provide.
2. **Timing.** The learner needs a whole dataset before it can update, and then it updates several times over that same data. If learning were inlined into the loop, that data would be discarded after one pass and you'd be forced into an online method.
3. **Boundaries.** Rewriting the reward, or swapping the policy, would mean editing the loop. Right now those are one-line changes in `rl_main.py`.
4. **Distribution.** Shuffling (`buffer.py:123`) breaks the correlation between consecutive simulator frames — envs 0 and 1 sitting next to each other in a minibatch are two robots at the same instant. That concern lives in the buffer, not the loop.

**Why so many layers around the environment.** Each layer absorbs one messy concern so the others stay clean: `vec_env` absorbs multiprocessing and the C++ binding; `obs_features` absorbs the simulator's dict layout and image size; `reward` absorbs the task definition; `buffer` absorbs the data-shape mismatch between collection and learning; `algorithm` absorbs the learning rule. The result is that `trainer.py` reads like a textbook RL loop, and each of the pieces around it can be replaced or deleted without touching the others.

---

# 5. Final mental model

```text
        rl_main.py          chooses which pieces run (every number comes from rl_config.py)
              │
              ▼
        trainer.py          owns WHEN: reset → collect → update → log
              │
    ┌─────────┼──────────────┬────────────────┐
    ▼         ▼              ▼                ▼
obs_features policy       reward           algorithm
    │         │              │                │
    │         │              │                │
     └────┬────┴──────────────┼────────────────┘
          ▼                   ▼
      buffer.py         vec_env.VecEnv  ──► voxel_sim (C++)
    (stores rollout)     (N processes)       (raylib world)
          │                   ▲    │                │
          │                   │    │ reward         │
          │                   │    │ computed       │
          │                   │    │ inside step    │
          └── batches ──► algorithm ◄── observations ┘
```

> **`rl_config.py`** is responsible for the numbers, and nothing else.
> **`rl_main.py`** is responsible for the wiring — which policy, which reward, which algorithm — and is the only bridge to `scripts/vector_env`.
> **`trainer.py`** is responsible for *when* everything happens, and is the only module that talks to the simulator.
> **`obs_features.py`** is responsible for the shape of what the learner sees: a pair of branches — `uint8` pixel frames with their layout intact, plus one flat `float32` state vector — assembled from whichever keys `rl_config.py` lists.
> **`policy.py`** is responsible for deciding what the robot does, and is where your network will live.
> **`reward.py`** is responsible for what a frame is worth, and is a plain function you can rewrite.
> **`buffer.py`** is responsible for storing one rollout of 256 transitions as separate pixel and state branches, and turning it into shuffled minibatches with discounted returns.
> **`algorithm.py`** is responsible for learning, and currently learns nothing on purpose.
> **`vec_env.VecEnv`** is responsible for running N simulators in parallel and stacking their observations.
> **Together:** the trainer asks the policy what to do, the VecEnv runs it in N parallel copies of the simulator, the reward scores each frame, the buffer remembers the rollout, and the algorithm gets a chance to learn from it — and every one of those five decisions can be changed by editing exactly one place.
