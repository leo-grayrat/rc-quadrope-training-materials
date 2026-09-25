import time

import mujoco
import mujoco.viewer


model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)

with mujoco.viewer.launch_passive(model, data) as viewer:
    while viewer.is_running():
        step_start = time.time()

        mujoco.mj_step(model, data)
        viewer.sync()

        time_left = model.opt.timestep - (time.time() - step_start)
        if time_left > 0:
            time.sleep(time_left)
