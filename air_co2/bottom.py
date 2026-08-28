"""底壳（Bottom）模型 — `result` 供 CQ-editor 预览 / 导出。

坐标约定：
  * 载板长 = X，宽 = Y。
  * 底板外侧面 Z=0，底板内侧面 Z=FLOOR_THICK。
  * 载板底面支撑面 Z = FLOOR_THICK + PCB_CLEAR。

结构：盒身(底+四壁) / 4 空心固定柱(含底孔+沉头) / +X 壁 USB-C 开槽。
"""

from __future__ import annotations

import cadquery as cq

from . import geometry as g

OUTER_L, OUTER_W = g.outer_dim()
INNER_L, INNER_W = g.inner_dim()
H = g.FLOOR_THICK + g.CAVITY_H          # 外框总高


def _box(l: float, w: float, h: float, z_bottom: float = 0.0) -> cq.Workplane:
    """以 (0,0,z_bottom) 为底面的 box。"""
    return cq.Workplane("XY").box(l, w, h, centered=(True, True, False)).translate((0, 0, z_bottom))


def build_bottom() -> cq.Workplane:
    body = _box(OUTER_L, OUTER_W, H)                      # 外框，底面 Z=0
    cavity = _box(INNER_L, INNER_W, g.CAVITY_H, z_bottom=g.FLOOR_THICK)
    body = body.cut(cavity)                                # 挖内腔（保留 FLOOR_THICK 底板）

    # ---- 固定柱：空心管（含内孔贯通到底）+ 底部沉头窝 ----
    pillar_top = g.FLOOR_THICK + g.PCB_CLEAR  # 载板底面承载面
    for (px, py) in g.PILLAR_POS:
        pillar = (
            cq.Workplane("XY", origin=(px, py, g.FLOOR_THICK))
            .circle(g.PILLAR_OD / 2)
            .extrude(pillar_top - g.FLOOR_THICK)
        )
        body = body.union(pillar)

        # 从柱顶向下开 M2.5 自攻导孔，底部保留封闭。
        hole_depth = pillar_top - g.FLOOR_THICK - 0.8
        hole = (
            cq.Workplane("XY", origin=(px, py, pillar_top + 0.1))
            .circle(g.PILOT_D / 2)
            .extrude(-(hole_depth + 0.1))
        )
        body = body.cut(hole)

    # ---- 内置软包电池托盘：位于 PCB 下方，四周限位但不压紧电芯 ----
    bat_l = g.BAT_LENGTH + 2 * g.BAT_GAP
    bat_w = g.BAT_WIDTH + 2 * g.BAT_GAP
    rail_z = g.FLOOR_THICK + g.BAT_PAD
    for y in (-(bat_w + g.BAT_RAIL) / 2, (bat_w + g.BAT_RAIL) / 2):
        rail = _box(bat_l, g.BAT_RAIL, g.BAT_RAIL_H, rail_z).translate((0, y, 0))
        body = body.union(rail)
    for x in (-(bat_l + g.BAT_RAIL) / 2, (bat_l + g.BAT_RAIL) / 2):
        rail = _box(g.BAT_RAIL, bat_w, g.BAT_RAIL_H, rail_z).translate((x, 0, 0))
        body = body.union(rail)

    # ---- +X 壁 USB-C 开槽（法向 X，穿过壁厚）----
    wall_x = g.USB_SIDE * (OUTER_L / 2 + 0.5)
    zc = g.FLOOR_THICK + g.USB_CENTER_ABOVE_FLOOR
    slot = (
        cq.Workplane("YZ", origin=(wall_x, g.USB_OFFSET_Y, zc))
        # 在该平面上第一维沿 Y、第二维沿 Z
        .rect(g.USB_W, g.USB_H)
        .extrude(-g.USB_SIDE * (g.WALL + 2.0))
    )
    body = body.cut(slot)

    # ---- -Y 壁 SW1 操作窗 ----
    sw_y = -OUTER_W / 2 - 0.5
    sw_z = g.FLOOR_THICK + g.SW_CENTER_ABOVE_FLOOR
    sw_cut = (
        cq.Workplane("XZ", origin=(g.SW_OFFSET_X, sw_y, sw_z))
        .rect(g.SW_W, g.SW_H)
        .extrude(-(g.WALL + 2.0))
    )
    body = body.cut(sw_cut)

    return body


result = build_bottom()
