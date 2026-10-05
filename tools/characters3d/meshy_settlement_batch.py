"""One authorized batch: five Meshy-6 geometry tasks for Settlement building
exteriors (Lumber Camp, Mine, Tannery, Herb Garden, Fishery). The Great Hall
is being generated separately (manually, via meshy.com) so it is not in this
batch - add it here later only if that one doesn't pan out.

Never rerun this paid batch. Resume a submitted task with meshy.py download.
"""
import json
import os
from pathlib import Path
import time
import meshy

BATCH = "townforge-settlement-20261005"
PROMPTS = {
    "lumber": "One standalone open-sided timber mill shed, a simple post-and-beam roofed work area (no walls on two sides), a large waist-height sawyer's bench under the roof, log ends stacked along the open back wall. No person, no tools in hand. Rough-hewn practical dark-fantasy sandbox MMO structure, weathered wood, modest scale. Front opening facing forward, single isolated building exterior, about 4000 triangles.",
    "mine": "One standalone mine entrance structure built into a low rock outcrop: a timber headframe and winch frame over a dark shaft opening, a short wooden rail track leading out of the entrance, rough stone retaining walls either side. No person, no ore cart. Grounded practical dark-fantasy sandbox MMO structure, weathered timber and stone. Entrance facing forward, single isolated structure, about 4000 triangles.",
    "tannery": "One standalone low timber tannery workshop exterior, wattle-and-daub walls, a shallow-pitched shingle roof, two large open wooden curing vats built into the ground beside the entrance, hide-drying frames mounted along one exterior wall. No person, no hides on the frames. Grounded practical dark-fantasy sandbox MMO structure, weathered and workaday. Front facing forward, single isolated building exterior, about 4000 triangles.",
    "herbs": "One standalone small timber-framed garden shelter, a simple lean-to roof over raised wooden planter beds, one open side, a drying rack of hanging bundles under the eave. No person, no plants rendered in the beds. Grounded practical dark-fantasy sandbox MMO structure, modest and rustic. Open side facing forward, single isolated structure, about 4000 triangles.",
    "fishery": "One standalone timber fishery shack built on short pilings, a single-room shed with a covered porch, net-drying racks along the porch railing, a small smoking rack beside the door. No person, no nets or fish rendered. Grounded practical dark-fantasy sandbox MMO structure, weathered dockside timber. Porch facing forward, single isolated building exterior, about 4000 triangles.",
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
    for name, prompt in PROMPTS.items():
        assert len(prompt) <= 800
        tasks[name] = {"status": "SUBMITTING", "max_credits": 20, "prompt": prompt}
        ledger.write_text(json.dumps(tasks, indent=2))
        payload = {"mode": "preview", "prompt": prompt, "ai_model": "meshy-6",
                   "should_remesh": True, "topology": "triangle", "target_polycount": 4000}
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
    print("Completed the five-building settlement geometry batch. No texturing requests submitted.")

if __name__ == "__main__":
    main()
