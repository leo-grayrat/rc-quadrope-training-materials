import argparse
import time

import mujoco
import mujoco.viewer


# 填入自己的关节名和初始角度。
INITIAL_JOINT_POSITIONS = {
    # "joint_name": angle_in_rad,
}

# 根据自己的模型调整。
INITIAL_BASE_HEIGHT = None


def set_hinge_joint(model, data, joint_name, position):
    joint_id = mujoco.mj_name2id(
        model, mujoco.mjtObj.mjOBJ_JOINT, joint_name
    )
    if joint_id < 0:
        raise ValueError(f"joint not found: {joint_name}")

    joint_type = model.jnt_type[joint_id]
    if joint_type != mujoco.mjtJoint.mjJNT_HINGE:
        raise ValueError(f"{joint_name} is not a hinge joint")

    qpos_adr = model.jnt_qposadr[joint_id]
    data.qpos[qpos_adr] = position


def set_free_base_height(model, data, height):
    free_joints = [
        joint_id
        for joint_id in range(model.njnt)
        if model.jnt_type[joint_id] == mujoco.mjtJoint.mjJNT_FREE
    ]

    if len(free_joints) != 1:
        raise ValueError(
            f"expected exactly one free joint, found {len(free_joints)}"
        )

    qpos_adr = model.jnt_qposadr[free_joints[0]]
    data.qpos[qpos_adr + 2] = height


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", help="scene MJCF path")
    args = parser.parse_args()

    model = mujoco.MjModel.from_xml_path(args.model)
    data = mujoco.MjData(model)

    if INITIAL_BASE_HEIGHT is not None:
        set_free_base_height(model, data, INITIAL_BASE_HEIGHT)

    for joint_name, position in INITIAL_JOINT_POSITIONS.items():
        set_hinge_joint(model, data, joint_name, position)

    mujoco.mj_forward(model, data)

    if model.nu > 0:
        data.ctrl[:] = 0.0

    with mujoco.viewer.launch_passive(model, data) as viewer:
        while viewer.is_running():
            step_start = time.time()

            # 本任务要求零力矩。不要在这里加入保持姿态的控制器。
            if model.nu > 0:
                data.ctrl[:] = 0.0

            mujoco.mj_step(model, data)
            viewer.sync()

            time_left = model.opt.timestep - (
                time.time() - step_start
            )
            if time_left > 0:
                time.sleep(time_left)


if __name__ == "__main__":
    main()
