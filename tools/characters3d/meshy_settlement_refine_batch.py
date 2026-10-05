"""One authorized batch: five Meshy-6 refine (texturing) tasks, building on the
approved townforge-settlement-20261005 preview geometry.

Never rerun this paid batch. Resume a submitted task with meshy.py download.
"""
import json
import os
from pathlib import Path
import time
import meshy

BATCH = "townforge-settlement-refine-20261005"
PREVIEW_TASKS = {
    "lumber": "01a10a70-a3ff-775d-9d14-cfd3bd465c6b",
    "mine": "01a10a70-abee-7007-bd72-2ee9d0f4cd53",
    "tannery": "01a10a70-b25f-71c5-8bae-c0cae5cb895a",
    "herbs": "01a10a70-c271-74df-8bd9-b7f7ce34f1d2",
    "fishery": "01a10a70-ce9c-74b8-b305-3224cffd389a",
}

def main():
    if os.environ.get("MESHY_APPROVED_BATCH") != BATCH:
        raise ValueError("This batch requires its explicit approval identifier.")
    if os.environ.get("GITHUB_RUN_ATTEMPT", "1") != "1":
        raise ValueError("Paid batch cannot be rerun. Download saved task IDs instead.")
    root = Path("meshy-output") / BATCH
    root.mkdir(parents=True, exist_ok=True)
    ledger = root / "tasks.json"
    if ledger.exists():
        raise ValueError("Batch already started. Resume individual task IDs.")
    tasks = {}
    ledger.write_text(json.dumps(tasks))
    endpoint = meshy.ENDPOINTS["text-to-3d"]
    for name, preview_id in PREVIEW_TASKS.items():
        preview = meshy.api(endpoint + "/" + preview_id)
        if preview.get("status") != "SUCCEEDED" or preview.get("type") != "text-to-3d-preview":
            raise ValueError(f"{name}: preview task is not a succeeded text-to-3d-preview.")
        tasks[name] = {"status": "SUBMITTING", "max_credits": 25, "preview_task_id": preview_id}
        ledger.write_text(json.dumps(tasks, indent=2))
        payload = {"mode": "refine", "preview_task_id": preview_id,
                   "target_formats": ["glb"], "enable_pbr": False}
        # One POST per piece. No retry after failure or an ambiguous response.
        ident = meshy.task_id(meshy.api(endpoint, payload)["result"])
        tasks[name].update(task_id=ident, status="PENDING")
        ledger.write_text(json.dumps(tasks, indent=2))
        print(f"Submitted {name}: {ident}", flush=True)
    deadline = time.monotonic() + 1500
    pending = set(tasks)
    failed = []
    while pending and time.monotonic() < deadline:
        for name in list(pending):
            task = meshy.api(endpoint + "/" + tasks[name]["task_id"])
            status = task.get("status")
            tasks[name].update(status=status, progress=task.get("progress"),
                               consumed_credits=task.get("consumed_credits"))
            ledger.write_text(json.dumps(tasks, indent=2))
            print(f"{name}: {status} {task.get('progress', '')}", flush=True)
            if status == "SUCCEEDED":
                meshy.stage(task, root / name)
                pending.remove(name)
            elif status in ("FAILED", "CANCELED"):
                failed.append(name)
                pending.remove(name)
        if pending:
            time.sleep(15)
    if failed or pending:
        raise ValueError("Batch incomplete; resume saved task IDs, never resubmit: " + ", ".join(failed + sorted(pending)))
    print("Completed the five-building settlement refine/texture batch.")

if __name__ == "__main__":
    main()
