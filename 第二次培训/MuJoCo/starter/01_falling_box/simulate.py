import time

import mujoco
import mujoco.viewer


model = mujoco.MjModel.from_xml_path("scene.xml")
data = mujoco.MjData(model)

with mujoco.viewer.launch_passive(model, data) as viewer:
    time.sleep(0.5)

    while viewer.is_running():
        step_start = time.perf_counter()

        mujoco.mj_step(model, data)
        viewer.sync()

        elapsed = time.perf_counter() - step_start
        remaining = model.opt.timestep - elapsed
        if remaining > 0:
            time.sleep(remaining)
