"""One authorized batch: three Meshy-6 geometry tasks, 60 credits total.

Never rerun this paid batch. Resume a submitted task with meshy.py download.
"""
import json
import os
from pathlib import Path
import time
import meshy

BATCH = "townforge-armor-20261003"
PROMPTS = {
    "chest": "One standalone wearable medieval steel cuirass, front breastplate and backplate joined at the sides, empty hollow interior, open neck and arm holes, no person or mannequin. Realistic adult male proportions, broad upper chest tapering to a fitted narrow waist. Shallow angular chest faces with a central ridge, beveled edges, restrained rivets, three overlapping waist lames. Thin armor shell, not a solid block. Dark fantasy sandbox MMO equipment, grounded practical forged steel, compact silhouette. No inflated chest, no spherical surfaces, no oversized trim, no spikes, no helmet, no arms, no legs. Symmetrical, upright with neck at top, front facing forward. Single isolated armor object, about 4000 triangles.",
    "shoulder": "One standalone right shoulder pauldron for an adult male medieval warrior. Empty hollow underside for the shoulder to fit inside, no person, no mannequin, no arm. Compact angular forged steel shoulder cap with beveled planar surfaces and three overlapping thin lames descending over the upper arm. Practical dark fantasy sandbox MMO gear, restrained rivets, matches an angular fitted steel cuirass. Anatomical snug shoulder proportions. No spherical ball shape, no inflated dome, no oversized silhouette, no spikes, no chestplate, no second shoulder. Upright shoulder cap above, upper-arm lames below, front facing forward. Single isolated wearable armor object, about 4000 triangles.",
    "greave": "One standalone right shin greave for an adult male medieval warrior. A fitted hollow steel shin guard with open top and bottom and open back secured by two leather straps. No person, no mannequin, no leg, no foot. Long tapered anatomical shin proportions, wider below the knee and narrow at the ankle. Angular front ridge, beveled edges, restrained rivets, compact practical forged steel. Grounded dark fantasy sandbox MMO equipment matching an angular steel cuirass and layered pauldron. Thin shell, not a solid cylinder, no rounded bulbous surfaces, no spikes, no boot, no second greave. Upright knee opening at top, ankle opening below, front facing forward. Single isolated wearable armor object, about 4000 triangles.",
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
    print("Completed the three-piece armor geometry batch. No texturing or rigging requests submitted.")

if __name__ == "__main__":
    main()
