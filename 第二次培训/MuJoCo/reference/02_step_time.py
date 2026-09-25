from pathlib import Path

import mujoco


scene = Path(__file__).resolve().parent.parent / "starter/01_falling_box/scene.xml"

model = mujoco.MjModel.from_xml_path(str(scene))
data = mujoco.MjData(model)

print(data.time)

for _ in range(10):
    mujoco.mj_step(model, data)

print(data.time)
