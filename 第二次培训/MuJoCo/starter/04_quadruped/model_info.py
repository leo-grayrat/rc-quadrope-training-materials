import argparse

import mujoco


def object_name(model, obj_type, obj_id):
    name = mujoco.mj_id2name(model, obj_type, obj_id)
    return name if name is not None else "<unnamed>"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", help="MJCF / XML model path")
    args = parser.parse_args()

    model = mujoco.MjModel.from_xml_path(args.model)

    print("== model ==")
    print("nq:", model.nq)
    print("nv:", model.nv)
    print("nu:", model.nu)
    print("njnt:", model.njnt)
    print("nactuator:", model.nactuator)

    print("\n== joints ==")
    for joint_id in range(model.njnt):
        name = object_name(model, mujoco.mjtObj.mjOBJ_JOINT, joint_id)
        print(
            f"{joint_id:2d}  "
            f"{name:24s}  "
            f"type={int(model.jnt_type[joint_id])}  "
            f"qpos_adr={int(model.jnt_qposadr[joint_id])}  "
            f"dof_adr={int(model.jnt_dofadr[joint_id])}"
        )

    print("\n== actuators ==")
    for actuator_id in range(model.nactuator):
        name = object_name(
            model, mujoco.mjtObj.mjOBJ_ACTUATOR, actuator_id
        )

        target = "<non-joint transmission>"
        if (
            model.actuator_trntype[actuator_id]
            == mujoco.mjtTrn.mjTRN_JOINT
        ):
            joint_id = int(model.actuator_trnid[actuator_id, 0])
            target = object_name(
                model, mujoco.mjtObj.mjOBJ_JOINT, joint_id
            )

        print(f"{actuator_id:2d}  {name:24s}  joint={target}")

    if model.nu != model.nactuator:
        print(
            "\nNOTE: nu != nactuator. "
            "Do not assume actuator id is the ctrl index."
        )


if __name__ == "__main__":
    main()
