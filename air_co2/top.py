"""顶壳（Top）模型 — `result` 供 CQ-editor 预览 / 导出。

坐标约定：顶壳底面（与底壳顶配合）在 Z = g.FLOOR_THICK + g.CAVITY_H（即底壳顶面）。
顶板外表面在 Z = base_z + TOP_THICK + LID_DEPTH。

结构：开口朝下的腔体 / 顶板屏幕窗 / SCD40 通气栅格 / 底部 4 处螺丝过孔。
"""

from __future__ import annotations

import cadquery as cq

from . import geometry as g

OUTER_L, OUTER_W = g.outer_dim()
INNER_L, INNER_W = g.inner_dim()
BASE_Z = g.FLOOR_THICK + g.CAVITY_H     # 与底壳配合面 Z

TOP_H = g.TOP_THICK + g.LID_DEPTH        # 顶壳鼓包总高


def _box(l: float, w: float, h: float, z_bottom: float) -> cq.Workplane:
    return cq.Workplane("XY").box(l, w, h, centered=(True, True, False)).translate((0, 0, z_bottom))


def build_top() -> cq.Workplane:
    # ---- 腔体：外框顶盒，挖出朝下的内腔（保留顶板 TOP_THICK）----
    body = _box(OUTER_L, OUTER_W, TOP_H, BASE_Z)
    # 内腔：x/y 比底壳略小配合，高度 = LID_DEPTH（从底面往上，去掉顶板）
    cavity_l = INNER_L - g.LIP_CLEARANCE
    cavity_w = INNER_W - g.LIP_CLEARANCE
    cavity = _box(cavity_l, cavity_w, g.LID_DEPTH + 0.2, BASE_Z - 0.1)
    body = body.cut(cavity)

    # 定位唇边向下伸入底壳，让上下壳不会横向错位。
    lip_outer = _box(cavity_l, cavity_w, g.LIP_H, BASE_Z - g.LIP_H)
    lip_inner = _box(
        cavity_l - 2 * g.LIP_WALL,
        cavity_w - 2 * g.LIP_WALL,
        g.LIP_H + 0.2,
        BASE_Z - g.LIP_H - 0.1,
    )
    body = body.union(lip_outer.cut(lip_inner))

    # ---- 屏幕窗：在顶板挖穿（相对载板中心的 LCD 偏移）----
    top_inner_z = BASE_Z + g.LID_DEPTH            # 顶板内表面
    lcd_center_x = g.LCD_OFFSET_X
    lcd_center_y = g.LCD_OFFSET_Y
    lcd_cut = _box(
        g.LCD_W + 0.6, g.LCD_H + 0.6, g.TOP_THICK + 0.6,
        z_bottom=top_inner_z - 0.3,
    ).translate((lcd_center_x, lcd_center_y, 0))
    lcd_cut = lcd_cut.edges("|Z").fillet(g.LCD_R)
    body = body.cut(lcd_cut)

    # ---- SCD40 通气栅格：顶板开一排细槽 ----
    scd_cx = g.SCD_OFFSET_X
    scd_cy = g.SCD_OFFSET_Y
    n = g.VENT_SLOTS
    pitch = g.VENT_SLOT_W + g.VENT_SLOT_GAP
    total = (n - 1) * pitch + g.VENT_SLOT_W
    # 栅格沿 X 排布，覆盖 SCD 投影宽度一段
    for i in range(n):
        x = scd_cx - total / 2 + g.VENT_SLOT_W / 2 + i * pitch
        slot = _box(g.VENT_SLOT_W, g.VENT_SLOT_H, g.TOP_THICK + 0.6, top_inner_z - 0.3).translate(
            (x, scd_cy, 0)
        )
        body = body.cut(slot)

    # ---- 螺丝过孔：对应底壳 4 固定柱，从顶板顶面穿到底（M2.5/M3）----
    for (px, py) in g.SCREW_POS:
        hole = (
            cq.Workplane("XY", origin=(px, py, BASE_Z + TOP_H))
            .circle(g.TOP_HOLE / 2)
            .extrude(-(TOP_H + g.LIP_H + 0.5))
        )
        body = body.cut(hole)

    return body


result = build_top()
