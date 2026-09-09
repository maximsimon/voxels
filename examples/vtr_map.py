#!/usr/bin/env python3
"""Load a VTR (visual teach & repeat) map recorded against the Voxel World simulator.

A map folder holds

    <dist>.jpg               camera frame captured that far along the taught path
    <dist>.npy               its learned representation (not needed to replay)
    bag/bag_0.mcap           /recorded_odometry + /recorded_actions for the whole run
    bag/metadata.yaml        rosbag2 metadata
    params                   topic names and the sampling step used during teaching

Only the bag matters for replaying the drive, so this module is a small self-contained
reader for it.  It deliberately does not depend on rosbag2, rclpy or the `mcap` package:
none of those are installed here, and the two message types involved are simple enough
that parsing them directly is less work than pulling in a ROS stack.

    from vtr_map import load_map
    m = load_map("examples/maps/my_first_map")
    m.poses        # (N, 3) float64  x, z, yaw  in simulator floor-plane coordinates
    m.actions      # (M, 3) float64  forward m/s, yaw rad/s, distance travelled m
    m.frames       # [(distance, jpg_path), ...] sorted by distance

Coordinate note: the bag was recorded through the ROS 2 bridge, so it is in REP-103 axes
(x forward, y left, z up) while the simulator itself works in raylib axes (x, z on the
floor, y up).  load_map() undoes that swap, so VtrMap.poses is already (x, z, yaw) in the
simulator's own floor-plane coordinates and can be handed straight to Reset()/Teleport().
Yaw needs no conversion: the bridge defines ROS yaw == raylib yaw.
"""

from __future__ import annotations

import math
import os
import struct
from dataclasses import dataclass

import numpy as np

# ---------------------------------------------------------------------------
# MCAP container
# ---------------------------------------------------------------------------

MCAP_MAGIC = b"\x89MCAP0\r\n"

_OP_SCHEMA = 0x03
_OP_CHANNEL = 0x04
_OP_MESSAGE = 0x05
_OP_CHUNK = 0x06


class _Reader:
    """Cursor over a bytes buffer, little-endian, with the MCAP primitive types."""

    def __init__(self, buf: bytes, pos: int = 0):
        self.buf = buf
        self.pos = pos

    def __len__(self):
        return len(self.buf) - self.pos

    def u8(self):
        v = self.buf[self.pos]
        self.pos += 1
        return v

    def u16(self):
        v = struct.unpack_from("<H", self.buf, self.pos)[0]
        self.pos += 2
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.buf, self.pos)[0]
        self.pos += 4
        return v

    def u64(self):
        v = struct.unpack_from("<Q", self.buf, self.pos)[0]
        self.pos += 8
        return v

    def bytes(self, n):
        v = self.buf[self.pos:self.pos + n]
        self.pos += n
        return v

    def string(self):
        return self.bytes(self.u32()).decode("utf-8", "replace")


def _iter_records(reader: _Reader):
    """Yield (opcode, payload) for every top-level record in the buffer."""
    while len(reader) >= 9:
        op = reader.u8()
        length = reader.u64()
        if length > len(reader):
            break  # truncated tail - stop rather than raise, the data so far is still good
        yield op, reader.bytes(length)


def read_mcap_messages(path: str, topics: set[str]):
    """Yield (topic, log_time_ns, payload_bytes) for messages on `topics`, in file order.

    Handles both loose Message records and Chunk records (rosbag2 writes chunks).  Only
    uncompressed chunks are supported - rosbag2 defaults to no compression and both maps
    here are written that way; anything else raises rather than silently returning nothing.
    """
    with open(path, "rb") as f:
        data = f.read()

    if not data.startswith(MCAP_MAGIC):
        raise ValueError(f"{path} is not an MCAP file")

    channels: dict[int, str] = {}

    def handle(op, payload):
        if op == _OP_CHANNEL:
            r = _Reader(payload)
            channel_id = r.u16()
            r.u16()  # schema_id
            channels[channel_id] = r.string()  # topic
        elif op == _OP_MESSAGE:
            r = _Reader(payload)
            channel_id = r.u16()
            r.u32()  # sequence
            log_time = r.u64()
            r.u64()  # publish_time
            topic = channels.get(channel_id)
            if topic in topics:
                return (topic, log_time, payload[r.pos:])
        return None

    reader = _Reader(data, len(MCAP_MAGIC))
    for op, payload in _iter_records(reader):
        if op == _OP_CHUNK:
            r = _Reader(payload)
            r.u64()  # message_start_time
            r.u64()  # message_end_time
            r.u64()  # uncompressed_size
            r.u32()  # uncompressed_crc
            compression = r.string()
            if compression:
                raise NotImplementedError(
                    f"{path}: chunk compression {compression!r} is not supported"
                )
            records_len = r.u64()
            inner = _Reader(r.bytes(records_len))
            for inner_op, inner_payload in _iter_records(inner):
                out = handle(inner_op, inner_payload)
                if out:
                    yield out
        else:
            out = handle(op, payload)
            if out:
                yield out


# ---------------------------------------------------------------------------
# CDR (the ROS 2 serialization inside each message)
# ---------------------------------------------------------------------------

class _CdrReader:
    """Minimal CDR decoder.

    Alignment is measured from the end of the 4-byte encapsulation header, which is what
    `self.origin` tracks.  Only the primitives the two message types here need are
    implemented.
    """

    def __init__(self, payload: bytes):
        if len(payload) < 4:
            raise ValueError("CDR payload too short")
        # encapsulation header: 0x00 0x00 = big endian, 0x00 0x01 = little endian
        self.little_endian = payload[1] in (1, 3)
        self.buf = payload
        self.origin = 4
        self.pos = 4

    @property
    def _e(self):
        return "<" if self.little_endian else ">"

    def _align(self, size):
        offset = (self.pos - self.origin) % size
        if offset:
            self.pos += size - offset

    def f32(self):
        self._align(4)
        v = struct.unpack_from(self._e + "f", self.buf, self.pos)[0]
        self.pos += 4
        return v

    def f64(self):
        self._align(8)
        v = struct.unpack_from(self._e + "d", self.buf, self.pos)[0]
        self.pos += 8
        return v

    def i32(self):
        self._align(4)
        v = struct.unpack_from(self._e + "i", self.buf, self.pos)[0]
        self.pos += 4
        return v

    def u32(self):
        self._align(4)
        v = struct.unpack_from(self._e + "I", self.buf, self.pos)[0]
        self.pos += 4
        return v

    def string(self):
        n = self.u32()                       # length includes the trailing NUL
        s = self.buf[self.pos:self.pos + n - 1] if n else b""
        self.pos += n
        return s.decode("utf-8", "replace")

    def skip(self, n):
        self.pos += n

    def f64_array(self, n):
        self._align(8)
        v = np.frombuffer(self.buf, dtype=self._e + "f8", count=n, offset=self.pos)
        self.pos += 8 * n
        return v


def decode_odometry(payload: bytes):
    """nav_msgs/msg/Odometry -> (stamp_seconds, x, y, z, qx, qy, qz, qw, vx, wz)."""
    r = _CdrReader(payload)
    sec = r.i32()
    nanosec = r.u32()
    r.string()                       # header.frame_id
    r.string()                       # child_frame_id
    x, y, z = r.f64(), r.f64(), r.f64()
    qx, qy, qz, qw = r.f64(), r.f64(), r.f64(), r.f64()
    r.f64_array(36)                  # pose covariance
    lx, _ly, _lz = r.f64(), r.f64(), r.f64()
    _ax, _ay, az = r.f64(), r.f64(), r.f64()
    return (sec + nanosec * 1e-9, x, y, z, qx, qy, qz, qw, lx, az)


def decode_distanced_twist(payload: bytes):
    """pfvtr/msg/DistancedTwist -> (linear_x, angular_z, distance)."""
    r = _CdrReader(payload)
    lx, _ly, _lz = r.f64(), r.f64(), r.f64()
    _ax, _ay, az = r.f64(), r.f64(), r.f64()
    distance = r.f32()
    return (lx, az, distance)


# ---------------------------------------------------------------------------
# the map itself
# ---------------------------------------------------------------------------

def quat_to_yaw(qx, qy, qz, qw):
    """Heading in the floor plane measured from +x, radians.

    The bag is in ROS REP-103 axes (see the module docstring), so this is the standard
    tf2 yaw-about-+Z extraction.  Under the simulator's axis convention that value is
    also the raylib heading, i.e. exactly what Reset()/Teleport() take as yaw.
    """
    return math.atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz))


@dataclass
class VtrMap:
    name: str
    path: str
    times: np.ndarray      # (N,)   seconds, from the odometry stamps
    poses: np.ndarray      # (N, 3) x, z, yaw(rad) in simulator floor-plane coordinates
    twists: np.ndarray     # (N, 2) recorded forward m/s and yaw rad/s per odometry sample
    actions: np.ndarray    # (M, 3) commanded forward m/s, yaw rad/s, distance travelled m
    frames: list           # [(distance_m, jpg_path), ...] sorted by distance

    @property
    def path_length(self) -> float:
        """Length of the taught path in meters, from the recorded odometry."""
        d = np.diff(self.poses[:, :2], axis=0)
        return float(np.hypot(d[:, 0], d[:, 1]).sum())

    def __repr__(self):
        return (f"VtrMap({self.name!r}, {len(self.poses)} odom samples, "
                f"{len(self.actions)} actions, {len(self.frames)} frames, "
                f"{self.path_length:.1f} m)")


def load_map(path: str) -> VtrMap:
    """Read a map folder into a VtrMap."""
    path = os.path.abspath(path)
    if not os.path.isdir(path):
        raise FileNotFoundError(f"no such map folder: {path}")

    bag = os.path.join(path, "bag", "bag_0.mcap")
    if not os.path.isfile(bag):
        raise FileNotFoundError(f"no bag at {bag}")

    odom, actions = [], []
    for topic, _log_time, payload in read_mcap_messages(
            bag, {"/recorded_odometry", "/recorded_actions"}):
        if topic == "/recorded_odometry":
            odom.append(decode_odometry(payload))
        else:
            actions.append(decode_distanced_twist(payload))

    if not odom:
        raise ValueError(f"{bag} contains no /recorded_odometry messages")

    times = np.array([o[0] for o in odom])
    poses = np.array([(o[1], o[2], quat_to_yaw(o[4], o[5], o[6], o[7])) for o in odom])
    twists = np.array([(o[8], o[9]) for o in odom])

    frames = sorted(
        (float(f[:-4]), os.path.join(path, f))
        for f in os.listdir(path) if f.endswith(".jpg")
    )

    return VtrMap(
        name=os.path.basename(path),
        path=path,
        times=times - times[0],
        poses=poses,
        twists=twists,
        actions=np.array(actions) if actions else np.zeros((0, 3)),
        frames=frames,
    )


if __name__ == "__main__":
    import sys

    here = os.path.dirname(os.path.abspath(__file__))
    targets = sys.argv[1:] or sorted(
        os.path.join(here, "maps", d) for d in os.listdir(os.path.join(here, "maps"))
        if os.path.isdir(os.path.join(here, "maps", d))
    )

    for target in targets:
        m = load_map(target)
        print(m)
        print(f"  duration       {m.times[-1]:.2f} s")
        print(f"  x range        {m.poses[:, 0].min():8.2f} .. {m.poses[:, 0].max():8.2f}")
        print(f"  z range        {m.poses[:, 1].min():8.2f} .. {m.poses[:, 1].max():8.2f}")
        print(f"  start pose     x={m.poses[0, 0]:.2f} z={m.poses[0, 1]:.2f} "
              f"yaw={math.degrees(m.poses[0, 2]):.1f} deg")
        print(f"  end pose       x={m.poses[-1, 0]:.2f} z={m.poses[-1, 1]:.2f} "
              f"yaw={math.degrees(m.poses[-1, 2]):.1f} deg")
        if len(m.actions):
            print(f"  forward speed  {m.actions[:, 0].min():.2f} .. {m.actions[:, 0].max():.2f} m/s")
            print(f"  yaw rate       {m.actions[:, 1].min():.2f} .. {m.actions[:, 1].max():.2f} rad/s")
            print(f"  action dist    {m.actions[:, 2].min():.2f} .. {m.actions[:, 2].max():.2f} m")
        print(f"  frames         {m.frames[0][0]:.2f} .. {m.frames[-1][0]:.2f} m")
        print()
