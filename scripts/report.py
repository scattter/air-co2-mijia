"""打印尺寸摘要，用于在切片/打样前核对几何参数是否符合实物。"""

from __future__ import annotations

from air_co2 import geometry as g


def main():
    print("air-co2-carrier-v1 外壳尺寸摘要")
    print("=" * 52)
    print(f"PCB             : {g.PCB_LENGTH} x {g.PCB_WIDTH} mm")
    print(f"内腔(长x宽)     : {g.PCB_LENGTH + 2*g.GAP} x {g.PCB_WIDTH + 2*g.GAP} mm")
    print(f"外轮廓(长x宽)   : {g.outer_dim()[0]} x {g.outer_dim()[1]} mm")
    print(f"壁厚 / 底板 / 顶板: {g.WALL} / {g.FLOOR_THICK} / {g.TOP_THICK} mm")
    print(f"底壳内腔高      : {g.CAVITY_H} mm  (顶面 Z={g.FLOOR_THICK+g.CAVITY_H})")
    print(f"顶壳内腔 / 定位唇边: {g.LID_DEPTH} / {g.LIP_H} mm")
    print(f"内置电池预留    : {g.BAT_LENGTH} x {g.BAT_WIDTH} x {g.BAT_THICK} mm")
    bat_top = g.FLOOR_THICK + g.BAT_PAD + g.BAT_THICK
    pcb_bottom = g.FLOOR_THICK + g.PCB_CLEAR
    print(f"电池顶部到 PCB 净空: {pcb_bottom - bat_top:.1f} mm")
    print("-" * 52)
    print("⚠️  下列关键尺寸请在打印前对照实物/1:1 PDF 复核：")
    print(f"  载板底面净空      PCB_CLEAR            = {g.PCB_CLEAR}")
    print(f"  叠高总高(实测)    STACK_HEIGHT         = {g.STACK_HEIGHT}")
    print(f"  LCD 可见区        LCD_W x LCD_H        = {g.LCD_W} x {g.LCD_H}  (偏移 {g.LCD_OFFSET_X},{g.LCD_OFFSET_Y})")
    print(f"  USB 开槽          {g.USB_W} x {g.USB_H}  侧壁={g.USB_SIDE:+d}X Y={g.USB_OFFSET_Y} 中心高={g.USB_CENTER_ABOVE_FLOOR}")
    print(f"  SW1 操作窗       {g.SW_W} x {g.SW_H}  X={g.SW_OFFSET_X} 中心高={g.SW_CENTER_ABOVE_FLOOR}")
    print(f"  固定柱孔坐标      PILLAR_POS           = {g.PILLAR_POS}")
    print(f"  SCD40 栅格偏移    SCD_OFFSET_X,Y       = {g.SCD_OFFSET_X},{g.SCD_OFFSET_Y}  {g.VENT_SLOTS} 槽")


if __name__ == "__main__":
    main()
