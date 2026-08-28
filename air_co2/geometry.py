"""air-co2-enclosure — 共享几何与尺寸参数。

针对 air-co2-carrier-v1 载板（70 x 35 mm）的上下分体外壳。

⚠️ 标注 [MEASURE] 的尺寸请务必用卡尺按实际板件/实物核对后修正。
   尤其是 Waveshare ESP32-S3-LCD-1.47B 叠放后的总高度，以及各安装孔的实际坐标，
   直接决定屏幕窗和 USB 口是否对准。默认值仅为便于先行出图验证。
"""

from __future__ import annotations

# ────────────────────────────── 外壳 / PCB 基础参数 ──────────────────────────────
WALL = 2.2                     # 壁厚 (mm)
GAP = 2.0                      # PCB 四周与内腔壁的间隙 (mm)
PCB_LENGTH = 70.0              # 载板长 (mm)
PCB_WIDTH = 35.0               # 载板宽 (mm)
PCB_THICK = 1.6                # 载板板厚 (mm)
FLOOR_THICK = 2.4              # 底壳底板厚度 (mm)
TOP_THICK = 2.4                # 顶壳顶板厚度 (mm)

# 载板下方放置内置软包电池，同时给焊点和导线留净空。
PCB_CLEAR = 8.0

# [MEASURE] 底壳内腔总高（不含底板）。内腔顶面 Z = FLOOR_THICK + CAVITY_H。
CAVITY_H = 27.0

# 顶壳鼓包内腔高与伸入底壳的定位唇边。
LID_DEPTH = 6.0
LIP_H = 3.0
LIP_WALL = 1.0

# ────────────────────────────── 内腔 / 外轮廓尺寸 ──────────────────────────────
def outer_dim() -> tuple[float, float]:
    """外轮廓 (长, 宽)。"""
    il = PCB_LENGTH + 2 * GAP
    iw = PCB_WIDTH + 2 * GAP
    return il + 2 * WALL, iw + 2 * WALL


def inner_dim() -> tuple[float, float]:
    """内腔 (长, 宽)。"""
    return PCB_LENGTH + 2 * GAP, PCB_WIDTH + 2 * GAP


# ────────────────────────────── 叠高组件 (需实测) ──────────────────────────────
# [MEASURE] 载板 + Waveshare 1.47B 板 + 排针 + LCD 屏顶面的总高度。
STACK_HEIGHT = 22.0            # 默认估测，务必实测

# 内置锂聚合物软包电池：默认按 603450 级别预留。
# 电池不用刚性压紧，XY 方向留出公差，Z 方向留出膨胀余量。
BAT_LENGTH = 50.0
BAT_WIDTH = 34.0
BAT_THICK = 6.0
BAT_GAP = 0.8
BAT_PAD = 0.6
BAT_RAIL = 1.0
BAT_RAIL_H = 2.4

# Waveshare 官方机械图的 LCD AA（有效显示区）。
LCD_W = 32.35
LCD_H = 17.39
LCD_R = 2.3

# [MEASURE] LCD 屏中心相对载板中心的偏移 (X, Y)。长向 X，宽向 Y。
LCD_OFFSET_X = -15.34
LCD_OFFSET_Y = 6.35

# [MEASURE] USB-C 口相对载板中心偏移与开槽尺寸。
USB_W = 11.0
USB_H = 6.0
USB_SIDE = -1                  # -1 为 -X 侧壁，+1 为 +X 侧壁
USB_OFFSET_Y = 11.5
# USB 口开槽中心距底壳内底面的高度。默认估测，须对上叠高后 USB-C 实测位置。
USB_CENTER_ABOVE_FLOOR = 8.0

# [MEASURE] PCB 边缘 SW1 的侧向操作窗。
SW_OFFSET_X = -7.0
SW_W = 12.0
SW_H = 10.0
SW_CENTER_ABOVE_FLOOR = 13.7

# ────────────────────────────── 固定柱 / 螺丝 ──────────────────────────────
# [MEASURE] 载板 4 个 2.7mm 安装孔坐标 (相对载板中心的 X, Y)。
PILLAR_POS = [(-32.0, -14.5), (32.0, -14.5), (-32.0, 14.5), (32.0, 14.5)]
SCREW_POS = [(-32.0, -14.5), (32.0, -14.5), (32.0, 14.5)]
PILLAR_OD = 5.2                # 外径
PILOT_D = 2.1                  # M2.5 自攻螺丝导孔
TOP_HOLE = 2.8                 # 顶壳与 PCB 的 M2.5 通孔

# ────────────────────────────── SCD40 通气栅格 ──────────────────────────────
# SCD40 模块在当前 PCB 上旋转后的投影与中心。
SCD_W = 13.4
SCD_H = 21.6
SCD_OFFSET_X = 20.3
SCD_OFFSET_Y = 7.3
VENT_SLOTS = 5
VENT_SLOT_W = 1.5
VENT_SLOT_GAP = 1.2
VENT_SLOT_H = 23.6

# ────────────────────────────── 装配间隙 ──────────────────────────────
LIP_CLEARANCE = 0.6            # 总尺寸差，等于单边 0.3 mm 配合间隙
