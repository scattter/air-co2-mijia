"""装配预览 — 将底壳与顶壳合并成一个彩色装配体，供 CQ-editor 总览配合。

`result` 为 cq.Assembly（含颜色），可导出 GLB 查看带色预览。
"""

from __future__ import annotations

import cadquery as cq

from . import bottom, top
from . import geometry as g

pcb_z = g.FLOOR_THICK + g.PCB_CLEAR
pcb = cq.Workplane("XY").box(
    g.PCB_LENGTH,
    g.PCB_WIDTH,
    g.PCB_THICK,
    centered=(True, True, False),
).translate((0, 0, pcb_z))
battery = cq.Workplane("XY").box(
    g.BAT_LENGTH,
    g.BAT_WIDTH,
    g.BAT_THICK,
    centered=(True, True, False),
).translate((0, 0, g.FLOOR_THICK + g.BAT_PAD))

result = (
    cq.Assembly()
    .add(bottom.result, name="bottom", color=cq.Color("steelblue"))
    .add(top.result, name="top", color=cq.Color("gray"))
    .add(pcb, name="pcb-preview", color=cq.Color("green"))
    .add(battery, name="battery-preview", color=cq.Color("orange"))
)
