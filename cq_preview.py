"""CQ-editor preview entrypoint."""

from pathlib import Path
import sys


ROOT = str(Path(__file__).resolve().parent)
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

from air_co2.assembly import battery, pcb
from air_co2 import bottom, top


show_object(
    bottom.result,
    name="bottom",
    options={"color": (70, 130, 180)},
)
show_object(
    top.result,
    name="top",
    options={"color": (180, 180, 180), "alpha": 0.55},
)
show_object(
    pcb,
    name="pcb-preview",
    options={"color": (35, 140, 75)},
)
show_object(
    battery,
    name="battery-preview",
    options={"color": (235, 145, 35)},
)
