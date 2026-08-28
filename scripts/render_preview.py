"""生成带 PCB/电池占位的外壳爆炸预览图。"""

from __future__ import annotations

import argparse
import os
import struct
import tempfile
from pathlib import Path

mpl_cache = Path(tempfile.gettempdir()) / "air_co2_mpl_cache"
mpl_cache.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(mpl_cache))
os.environ.setdefault("XDG_CACHE_HOME", str(mpl_cache))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Patch, Rectangle
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from air_co2 import geometry as g


def read_stl(path: Path, z=0.0):
    record = struct.Struct("<12fH")
    faces = []
    with path.open("rb") as stream:
        stream.read(80)
        count = struct.unpack("<I", stream.read(4))[0]
        for _ in range(count):
            values = record.unpack(stream.read(record.size))
            verts = values[3:12]
            faces.append([
                (verts[i], verts[i + 1], verts[i + 2] + z)
                for i in range(0, 9, 3)
            ])
    return faces


def box_faces(size, center):
    sx, sy, sz = (v / 2 for v in size)
    cx, cy, cz = center
    points = [
        (cx + x * sx, cy + y * sy, cz + z * sz)
        for x, y, z in (
            (-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
            (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1),
        )
    ]
    return [
        [points[i] for i in face]
        for face in (
            (0, 1, 2, 3), (4, 5, 6, 7), (0, 1, 5, 4),
            (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7),
        )
    ]


def add_mesh(ax, faces, color, alpha, edge=False):
    edgecolor = (0, 0, 0, 0.16) if edge else "none"
    mesh = Poly3DCollection(faces, facecolor=color, edgecolor=edgecolor)
    mesh.set_linewidth(0.18 if edge else 0)
    mesh.set_alpha(alpha)
    ax.add_collection3d(mesh)


def setup_ax(ax, outer_l, outer_w, title, elev, azim, zmax):
    ax.set_xlim(-outer_l / 2 - 4, outer_l / 2 + 4)
    ax.set_ylim(-outer_w / 2 - 4, outer_w / 2 + 4)
    ax.set_zlim(0, zmax)
    ax.set_box_aspect((outer_l, outer_w, zmax))
    ax.view_init(elev=elev, azim=azim)
    ax.set_axis_off()
    ax.set_title(title, pad=8, fontsize=14)


def main():
    ap = argparse.ArgumentParser(description="渲染外壳爆炸预览 PNG")
    ap.add_argument("-o", "--out", default="out/preview.png")
    args = ap.parse_args()

    out = Path(args.out).resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    root = Path(__file__).resolve().parents[1]

    bottom = read_stl(root / "out/bottom.stl")
    top_raw = read_stl(root / "out/top.stl")
    top = read_stl(root / "out/top.stl", z=16.0)
    pcb_z = g.FLOOR_THICK + g.PCB_CLEAR + g.PCB_THICK / 2 + 29.0
    pcb = box_faces((g.PCB_LENGTH, g.PCB_WIDTH, g.PCB_THICK), (0, 0, pcb_z))
    bat_z = g.FLOOR_THICK + g.BAT_PAD + g.BAT_THICK / 2 + 28.0
    battery = box_faces((g.BAT_LENGTH, g.BAT_WIDTH, g.BAT_THICK), (8, -10, bat_z))

    outer_l, outer_w = g.outer_dim()
    fig = plt.figure(figsize=(16, 7), facecolor="#f7f7f7")
    iso = fig.add_subplot(131, projection="3d", facecolor="#f7f7f7")
    lid = fig.add_subplot(132, projection="3d", facecolor="#f7f7f7")
    tray = fig.add_subplot(133, facecolor="#f7f7f7")

    add_mesh(iso, bottom, "#2f75b5", 0.94)
    add_mesh(iso, top, "#b8bdc7", 0.68)
    add_mesh(iso, pcb, "#198754", 0.95, edge=True)
    add_mesh(iso, battery, "#f28e2b", 0.95, edge=True)
    setup_ax(iso, outer_l, outer_w, "Exploded", 27, -55, 58)
    iso.legend(
        handles=[
            Patch(color="#2f75b5", label="Bottom"),
            Patch(color="#b8bdc7", label="Top shell"),
            Patch(color="#198754", label="PCB envelope"),
            Patch(color="#f28e2b", label="Battery (offset)"),
        ],
        loc="upper left",
        frameon=False,
        fontsize=9,
    )

    add_mesh(lid, top_raw, "#b8bdc7", 0.96)
    setup_ax(lid, outer_l, outer_w, "Top shell", 90, -90, 42)
    tray.add_patch(Rectangle(
        (-outer_l / 2, -outer_w / 2), outer_l, outer_w,
        facecolor="#2f75b5", alpha=0.26, edgecolor="#2f75b5", linewidth=1.5,
    ))
    inner_l, inner_w = g.inner_dim()
    tray.add_patch(Rectangle(
        (-inner_l / 2, -inner_w / 2), inner_l, inner_w,
        facecolor="#f7f7f7", edgecolor="#2f75b5", linewidth=1.2,
    ))
    tray.add_patch(Rectangle(
        (-g.BAT_LENGTH / 2, -g.BAT_WIDTH / 2), g.BAT_LENGTH, g.BAT_WIDTH,
        facecolor="#f28e2b", alpha=0.48, edgecolor="#d87518", linewidth=1.2,
    ))
    tray.add_patch(Rectangle(
        (-g.PCB_LENGTH / 2, -g.PCB_WIDTH / 2), g.PCB_LENGTH, g.PCB_WIDTH,
        fill=False, edgecolor="#198754", linewidth=1.4, linestyle="--",
    ))
    for px, py in g.PILLAR_POS:
        tray.add_patch(Circle((px, py), g.PILLAR_OD / 2, color="#2f75b5"))
        tray.add_patch(Circle((px, py), g.PILOT_D / 2, color="#f7f7f7"))
    tray.plot(
        [-outer_l / 2, -outer_l / 2],
        [g.USB_OFFSET_Y - g.USB_W / 2, g.USB_OFFSET_Y + g.USB_W / 2],
        color="#d62728", linewidth=5, solid_capstyle="round",
    )
    tray.plot(
        [g.SW_OFFSET_X - g.SW_W / 2, g.SW_OFFSET_X + g.SW_W / 2],
        [-outer_w / 2, -outer_w / 2],
        color="#d62728", linewidth=5, solid_capstyle="round",
    )
    tray.set_xlim(-outer_l / 2 - 4, outer_l / 2 + 4)
    tray.set_ylim(-outer_w / 2 - 4, outer_w / 2 + 4)
    tray.set_aspect("equal")
    tray.set_axis_off()
    tray.set_title("Bottom layout", pad=8, fontsize=14)

    fig.tight_layout()
    fig.savefig(out, dpi=180, bbox_inches="tight", pad_inches=0.12)
    plt.close(fig)
    print(f"已渲染: {out}")


if __name__ == "__main__":
    main()
