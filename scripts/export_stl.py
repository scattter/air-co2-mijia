"""一键导出底壳/顶壳为 STL（MeshLib 格式，供 Bambu Studio 切片）。"""

from __future__ import annotations

import argparse
import os

import cadquery as cq

from air_co2 import bottom, top


def export(shape, out_path: str):
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    if isinstance(shape, cq.Assembly):
        shape = shape.toCompound()
    cq.exporters.export(shape, out_path)
    print(f"已导出: {os.path.abspath(out_path)}")


def main():
    ap = argparse.ArgumentParser(description="导出 air-co2 外壳 STL")
    ap.add_argument("part", choices=["bottom", "top", "assembly"])
    ap.add_argument(
        "-o", "--out", default="out",
        help="输出目录（默认 ./out），文件名自动为 bottom.stl / top.stl",
    )
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    if args.part in ("bottom", "assembly"):
        export(bottom.result, os.path.join(args.out, "bottom.stl"))
    if args.part in ("top", "assembly"):
        export(top.result, os.path.join(args.out, "top.stl"))


if __name__ == "__main__":
    main()
