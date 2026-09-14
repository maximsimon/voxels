#!/usr/bin/env bash
#
# Run the informed repeat over both taught maps under a range of parameter values and
# print a comparison table.
#
#   ./examples/repeat_sweep.sh
#   VOXELS_DETAIL=1 ./examples/repeat_sweep.sh     # also show each map separately
#   VOXELS_REPEATS=3 ./examples/repeat_sweep.sh    # best of N runs per configuration
#   VOXELS_BUILD=<dir> ./examples/repeat_sweep.sh  # a build directory other than build/
#   VOXELS_PYTHON=<exe> ./examples/repeat_sweep.sh # a specific interpreter
#
# The robot camera is fixed at 512x320 for every row, so the rows differ only in the
# parameter named. Every configuration produces a full observation - camera_front image,
# pose, yaw, velocities and lidar.
#
# The third-person window is off throughout: only the agent camera is rendered. The main
# window is never mapped either, which costs nothing to skip (measured at 0.06 ms/step
# without the camera, against 0.67 ms/step with it).
#
# Each row varies one parameter away from the baseline, so the columns are comparable.
# Wall time and steps/s cover both maps together; xte is the cross-track error against the
# taught path, weighted across both.

set -euo pipefail

# This machine's locale uses a decimal comma, which makes printf/awk reject and emit
# "115.34"-style numbers inconsistently. Pin the numeric formatting for the whole script.
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
cd "$ROOT"                       # the simulator resolves its resources relative to the repo root

# Overrides are VOXELS_-prefixed on purpose. A bare BUILD is not safe to read: conda's
# compiler packages export BUILD=x86_64-conda-linux-gnu when an environment is activated,
# which would silently point this script at a directory that does not exist.
REPEATS="${VOXELS_REPEATS:-${REPEATS:-2}}"     # best of N, to take the edge off run-to-run noise
DETAIL="${VOXELS_DETAIL:-${DETAIL:-0}}"
CAMERA=512x320                                 # fixed - see the note above

# Locate the build directory: an explicit override, then the usual name, then any directory
# here that actually contains the built module.
find_build_dir() {
	local d
	for d in ${VOXELS_BUILD:+"$VOXELS_BUILD"} build; do
		if compgen -G "$d/voxel_sim*.so" > /dev/null; then
			echo "$d"
			return 0
		fi
	done
	for d in */; do
		if compgen -G "${d}voxel_sim*.so" > /dev/null; then
			echo "${d%/}"
			return 0
		fi
	done
	return 1
}

if ! BUILD_DIR="$(find_build_dir)"; then
	echo "error: no voxel_sim module found under $ROOT" >&2
	echo "       looked in: ${VOXELS_BUILD:+$VOXELS_BUILD/, }build/, and every directory here" >&2
	echo "       build it first:" >&2
	echo "         cmake -S . -B build -DPython3_EXECUTABLE=/usr/bin/python3" >&2
	echo "         cmake --build build -j" >&2
	echo "       or point at an existing build with VOXELS_BUILD=<dir>" >&2
	exit 1
fi
export PYTHONPATH="$BUILD_DIR${PYTHONPATH:+:$PYTHONPATH}"

# Pick an interpreter that can actually import the built module. The extension is tied to
# one CPython version (voxel_sim.cpython-311-*.so needs 3.11), and the `python3` first on
# PATH is not necessarily that one - a conda base env in front of /usr/bin is enough to
# break it - so derive the version from the module filename and verify the import.
pick_python() {
	local candidates=() so tag p cached
	[ -n "${VOXELS_PYTHON:-}" ] && candidates+=("$VOXELS_PYTHON")

	# the interpreter cmake actually configured against - the most reliable answer, and not
	# necessarily the `python3` first on PATH (an activated conda env is enough to differ)
	if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
		cached="$(sed -n 's/^Python3_EXECUTABLE:[^=]*=//p' "$BUILD_DIR/CMakeCache.txt" | head -1)"
		[ -n "$cached" ] && candidates+=("$cached")
	fi

	# failing that, the version baked into the extension name (voxel_sim.cpython-311-*.so)
	for so in "$BUILD_DIR"/voxel_sim*.so; do
		tag="$(basename "$so" | sed -n 's/.*cpython-3\([0-9]\{1,\}\).*/3.\1/p')"
		[ -n "$tag" ] && candidates+=("/usr/bin/python$tag" "python$tag")
	done
	candidates+=(python3 /usr/bin/python3)

	for p in "${candidates[@]}"; do
		command -v "$p" > /dev/null 2>&1 || continue
		if "$p" -c 'import voxel_sim' > /dev/null 2>&1; then
			echo "$p"
			return 0
		fi
	done
	return 1
}

if ! PYTHON="$(pick_python)"; then
	echo "error: no python interpreter here can import the module in '$BUILD_DIR/'." >&2
	echo "       built module: $(basename "$(compgen -G "$BUILD_DIR"/voxel_sim*.so | head -1)")" >&2
	echo "       set VOXELS_PYTHON=/path/to/python to choose one explicitly." >&2
	exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Run the repeat once over both maps and emit its per-map TSV rows.
# The simulator writes raylib warnings to stdout, so keep only well-formed 8-field rows.
run_once() {
	"$PYTHON" examples/python_informed_repeat.py --tsv --camera-size "$CAMERA" "$@" 2>/dev/null \
		| awk -F'\t' 'NF==8'
}

# Aggregate per-map rows into one line: driven steps stepsPerM wall stepsPerS xteMean xteMax
aggregate() {
	awk -F'\t' '
		{ driven += $3; steps += $4; wall += $5; xsum += $7 * $4; if ($8 > xmax) xmax = $8 }
		END {
			if (steps == 0) { print "0 0 0 0 0 0 0"; exit }
			printf "%.4f %d %.4f %.4f %.4f %.4f %.4f\n",
			       driven, steps, steps / driven, wall, steps / wall, xsum / steps, xmax
		}'
}

# row <label> [extra args to python_informed_repeat.py ...]
row() {
	local label="$1"; shift

	: > "$TMP/runs"
	local i
	for ((i = 0; i < REPEATS; i++)); do
		if ! run_once "$@" > "$TMP/raw.$i"; then
			: > "$TMP/raw.$i"
		fi
		[ -s "$TMP/raw.$i" ] || continue
		aggregate < "$TMP/raw.$i" >> "$TMP/runs"
	done

	# keep the fastest repeat (column 4 is total wall time)
	local best
	best="$(sort -k4,4g "$TMP/runs" | head -1)"
	if [ -z "$best" ] || [ "${best%% *}" = "0.0000" ]; then
		printf "%-34s %s\n" "$label" "FAILED - rerun that config by hand to see the error"
		return
	fi

	# shellcheck disable=SC2086
	set -- $best
	printf "%-34s %8.1fm %7d %8.2f %8.3fs %9.1f %8.3fm %8.3fm\n" \
		"$label" "$1" "$2" "$3" "$4" "$5" "$6" "$7"

	if [ "$DETAIL" = "1" ]; then
		awk -F'\t' '{ printf "    %-30s %8.1fm %7d %8.2f %8.3fs %9.1f %8.3fm %8.3fm\n",
		              $1, $3, $4, $4/$3, $5, $6, $7, $8 }' "$TMP/raw.0"
	fi
}

header() {
	printf "\n%s\n" "$1"
	printf -- "%.0s-" {1..102}; printf "\n"
}

echo "informed repeat parameter sweep - both taught maps, agent camera at ${CAMERA}"
echo "build: $BUILD_DIR, interpreter: $PYTHON, best of ${REPEATS} run(s) per configuration"
echo "every row produces a full observation: camera image, pose, yaw, velocities, lidar"
echo "baseline: agent camera only (no third-person view), grid collisions, 5 steps/m,"
echo "          3.0 m/s, 180 lidar rays"
echo "each section varies one parameter; the '(default)' row in each is the baseline again,"
echo "so the spread across those rows shows the run-to-run noise floor"

# Show what one observation actually contains at this camera size, so the table cannot be
# mistaken for a run with sensors switched off.
printf "\nobservation at %s:\n" "$CAMERA"
{ "$PYTHON" - "$CAMERA" <<'PYEOF' || echo "  (could not sample an observation)"
import sys
import numpy as np
import voxel_sim

w, h = (int(v) for v in sys.argv[1].lower().split("x"))
voxel_sim.Configure(verbose=False, target_fps=0, render_gui=False, show_window=False,
                    keyboard_enabled=False, camera_width=w, camera_height=h)
obs = voxel_sim.Init()
for k, v in obs.items():
    if isinstance(v, np.ndarray):
        print(f"  {k:<13} {str(v.dtype):<8} shape {v.shape}")
    else:
        print(f"  {k:<13} {type(v).__name__:<8} {v}")
voxel_sim.Close()
PYEOF
} | grep -v '^\(INFO\|WARNING\):'

# discard one run so the first measured row is not the one paying GPU/driver warmup
printf "\nwarming up... "
run_once > /dev/null 2>&1 || true
echo "done"

printf "\n%-34s %9s %7s %8s %9s %9s %9s %9s\n" \
	"configuration" "driven" "steps" "steps/m" "wall" "steps/s" "xte mean" "xte max"

header "collision mode"
row "grid footprint (default)"
row "mesh footprint" --mesh-collisions

header "path sampling density (steps per meter)"
row "5 steps/m (default)" --steps-per-meter 5
row "10 steps/m" --steps-per-meter 10
row "20 steps/m" --steps-per-meter 20

header "commanded speed"
row "1.0 m/s" --speed 1.0
row "2.0 m/s" --speed 2.0
row "3.0 m/s (default, taught max)" --speed 3.0

header "lidar rays (part of the observation)"
row "180 rays (default)"
row "60 rays" --lidar-rays 60

printf -- "%.0s-" {1..102}; printf "\n"
echo "steps/m is a floor: the controller only ever slows below the commanded speed, so"
echo "turns are sampled more finely than the requested density, never less."
