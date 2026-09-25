import mujoco

model = mujoco.MjModel.from_xml_path("../starter/01_falling_box/scene.xml")
data = mujoco.MjData(model)

print(data.time)

for _ in range(10):
    mujoco.mj_step(model, data)

print(data.time)
