import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TRACE = ROOT / "trace.json"

EXPECTED = {
    "entry_file": "simulate_python/unitree_mujoco.py",
    "config_file": "simulate_python/config.py",
    "simulation_thread": "SimulationThread",
    "viewer_thread": "PhysicsViewerThread",
    "command_path": [
        "LowCmdHandler",
        "mj_data.ctrl",
        "mujoco.mj_step",
    ],
    "state_path": [
        "mj_data.sensordata",
        "PublishLowState",
        "low_state_puber.Write",
    ],
}


def main():
    data = json.loads(TRACE.read_text(encoding="utf-8"))

    failures = []
    for key, expected in EXPECTED.items():
        actual = data.get(key)
        if actual != expected:
            failures.append((key, expected, actual))

    if failures:
        for key, expected, actual in failures:
            print(f"[FAIL] {key}")
            print(f"  expected: {expected!r}")
            print(f"  actual:   {actual!r}")
        raise SystemExit(1)

    print("[PASS] unitree_mujoco source trace")


if __name__ == "__main__":
    main()
