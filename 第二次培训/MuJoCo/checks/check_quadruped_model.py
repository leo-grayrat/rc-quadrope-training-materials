import argparse
import math

import mujoco


def report(ok, message):
    tag = "PASS" if ok else "FAIL"
    print(f"[{tag}] {message}")
    return ok


def all_finite(values):
    return all(math.isfinite(float(value)) for value in values)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scene", help="MJCF scene file to load")
    args = parser.parse_args()

    try:
        model = mujoco.MjModel.from_xml_path(args.scene)
        data = mujoco.MjData(model)
    except Exception as exc:
        print(f"[FAIL] model loads: {exc}")
        raise SystemExit(1)

    results = []
    results.append(report(True, "model loads"))

    free_joint_count = sum(
        int(joint_type) == int(mujoco.mjtJoint.mjJNT_FREE)
        for joint_type in model.jnt_type
    )
    results.append(
        report(free_joint_count == 1, f"exactly one free base joint (found {free_joint_count})")
    )

    results.append(report(model.nu == 12, f"12 actuator inputs (found {model.nu})"))

    driven_joint_ids = []
    joint_transmissions_ok = True
    for actuator_id in range(model.nu):
        if int(model.actuator_trntype[actuator_id]) != int(mujoco.mjtTrn.mjTRN_JOINT):
            joint_transmissions_ok = False
            continue
        driven_joint_ids.append(int(model.actuator_trnid[actuator_id, 0]))

    results.append(report(joint_transmissions_ok, "every actuator directly drives a joint"))
    results.append(
        report(
            len(driven_joint_ids) == 12 and len(set(driven_joint_ids)) == 12,
            "12 actuators target 12 distinct joints",
        )
    )

    results.append(report(all_finite(data.qpos), "initial qpos is finite"))
    results.append(report(all_finite(data.qvel), "initial qvel is finite"))
    results.append(
        report(
            all(abs(float(value)) < 1e-12 for value in data.ctrl),
            "initial ctrl is all zero",
        )
    )

    for _ in range(200):
        mujoco.mj_step(model, data)

    results.append(report(all_finite(data.qpos), "qpos stays finite for 200 zero-input steps"))
    results.append(report(all_finite(data.qvel), "qvel stays finite for 200 zero-input steps"))

    if not all(results):
        raise SystemExit(1)

    print("[PASS] quadruped model structural checks")


if __name__ == "__main__":
    main()
