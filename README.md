# air-co2-enclosure — CadQuery 盒子模型

针对 `air-co2-carrier-v1` 载板（**70 × 35 mm**，双层）的 3D 打印外壳模型。默认方案将一块约 **50 × 34 × 6 mm** 的软包电池放在 PCB 下方。
模型用 **CadQuery（Python 代码参数化建模）** 编写，可在 **CQ-editor** 中预览 / 调参 / 导出。

尺寸基线来自本地 `air-co2-web.eprj2` 中的 `air-co2-carrier-v1-pcb`，屏幕外形与有效显示区参考 [Waveshare 官方 3D 结构包](https://www.waveshare.net/w/upload/2/2e/ESP32-S3-LCD-1.47B_20260204.zip)。

## 目录结构

```
models/
├── pyproject.toml          # uv 项目声明（依赖 cadquery）
├── air_co2/
│   ├── geometry.py         # ★ 所有尺寸参数集中在此（改参数的地方）
│   ├── bottom.py           # 底壳模型（result 变量）
│   ├── top.py              # 顶壳模型（result 变量）
│   ├── assembly.py         # 上下壳彩色装配预览
│   └── __init__.py
└── scripts/
    ├── export_stl.py       # 导出 STL（bottom / top / assembly）
    ├── check_model.py      # 几何与干涉自检
    ├── render_preview.py   # 生成爆炸预览图
    └── report.py           # 打印尺寸摘要供核对
```

## 在 CQ-editor 中观看 / 调参

1. 打开 CQ-editor，再打开项目根目录的 `cq_preview.py`，点击运行即可查看上下壳、PCB 和电池的彩色总览。
2. 需要改尺寸 / 开口位置时，**改 `geometry.py` 里带 `[MEASURE]` 的参数**，保存后 CQ-editor 自动刷新。
3. 在 CQ-editor 里确认模型无误后导出 STL（或用它内置的 export）。

## ⚠️ 使用前务必实测并填好的关键尺寸（都在 geometry.py）

| 参数 | 作用 | 状态 |
|------|------|------|
| `STACK_HEIGHT` | 载板+屏开发板+排针总高 → 决定盒高 | [MEASURE] 需你卡尺实测 |
| `LCD_W / LCD_H / LCD_OFFSET_*` | 屏幕窗位置尺寸 | 已按 Waveshare 官方 LCD AA 与 PCB 坐标设置 |
| `USB_OFFSET_Y / USB_CENTER_ABOVE_FLOOR` | USB 口位置 | [MEASURE] 需对叠高后实测 |
| `SW_*` | 电源拨动开关操作窗 | [MEASURE] 需对实物拨杆高度 |
| `PILLAR_POS` | 载板 4 个 2.7mm 安装孔坐标 | 已按 PCB 孔位设置，打印前仍建议实物复核 |
| `SCD_OFFSET_*` | SCD40 通气栅格位置 | 已按 PCB 禁布区设置 |
| `BAT_LENGTH / BAT_WIDTH / BAT_THICK` | 内置软包电池外形 | [MEASURE] 下单电池后需卡尺实测 |

> 默认值是为"先能出图验证几何逻辑"而估的，**不代表最终可用尺寸**。务必打印 `1:1 PDF` 对着实物修正。软包电池托盘只做限位，装配时使用绝缘泡棉胶固定，不要让螺丝、柱位或壳体挤压电芯。

上下壳默认用 3 根 M2.5 长螺丝从顶部穿过顶壳和 PCB，锁入底壳柱位的 2.1 mm 自攻导孔。左上角因紧邻屏幕窗不放顶壳螺丝，但仍保留 PCB 支撑柱。螺丝长度需按实际叠高选择，打印首件后再定。

## 命令行导出（可选，若配了 uv 环境）

```bash
uv run --with cadquery python scripts/report.py            # 看尺寸摘要
uv run --with cadquery python scripts/check_model.py       # 检查关键间隙/实体/干涉
uv run --with cadquery python scripts/export_stl.py bottom  # 导出底壳 STL 到 ./out
uv run --with cadquery python scripts/export_stl.py top     # 导出顶壳 STL
uv run --with cadquery python scripts/render_preview.py     # 渲染 ./out/preview.png
```

导出的 STL 直接拖进 **Bambu Studio** 切片 → 发 **拓竹 P1S + AMS** 打印（建议 PETG）。
