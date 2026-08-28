"""外壳的轻量几何自检：检查关键间隙、实体有效性和上下壳干涉。"""

from __future__ import annotations

from air_co2 import bottom, geometry as g, top


def main():
    bat_l = g.BAT_LENGTH + 2 * (g.BAT_GAP + g.BAT_RAIL)
    bat_w = g.BAT_WIDTH + 2 * (g.BAT_GAP + g.BAT_RAIL)
    inner_l, inner_w = g.inner_dim()
    assert bat_l < inner_l and bat_w < inner_w, "电池托盘超出内腔"

    bat_top = g.FLOOR_THICK + g.BAT_PAD + g.BAT_THICK
    pcb_bottom = g.FLOOR_THICK + g.PCB_CLEAR
    assert pcb_bottom - bat_top >= 1.0, "电池与 PCB 净空不足 1 mm"

    stack_top = pcb_bottom + g.STACK_HEIGHT
    lid_inner = g.FLOOR_THICK + g.CAVITY_H + g.LID_DEPTH
    assert lid_inner - stack_top >= 2.0, "屏幕/元件顶部净空不足 2 mm"

    for name, model in (("bottom", bottom.result), ("top", top.result)):
        shape = model.val()
        assert shape.isValid(), f"{name} 实体无效"
        assert shape.Volume() > 0, f"{name} 体积异常"

    overlap = bottom.result.val().intersect(top.result.val()).Volume()
    assert overlap < 0.01, f"上下壳干涉体积 {overlap:.3f} mm³"

    print("外壳自检通过")
    print(f"  电池到 PCB 净空: {pcb_bottom - bat_top:.1f} mm")
    print(f"  叠高到顶壳净空: {lid_inner - stack_top:.1f} mm")
    print(f"  上下壳干涉体积: {overlap:.3f} mm³")


if __name__ == "__main__":
    main()
