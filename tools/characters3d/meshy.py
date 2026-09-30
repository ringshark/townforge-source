"""Meshy asset staging. Credentials stay in the runner; assets are reviewed separately."""
import base64
import json
import os
from pathlib import Path
import re
import struct
import time
import urllib.error
import urllib.parse
import urllib.request

API = "https://api.meshy.ai"
ENDPOINTS = {"text-to-3d": "/openapi/v2/text-to-3d",
             "image-to-3d": "/openapi/v1/image-to-3d",
             "rigging": "/openapi/v1/rigging",
             "animations": "/openapi/v1/animations"}


def task_id(value):
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,128}", value):
        raise ValueError("Enter a Meshy task ID, not a URL.")
    return value


def api(path, payload=None):
    key = os.environ.get("MESHY_API_KEY", "").strip()
    if not key:
        raise ValueError("Add the MESHY_API_KEY repository secret first.")
    request = urllib.request.Request(API + path,
        data=None if payload is None else json.dumps(payload).encode(),
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        # Never echo upstream response bodies, headers, or signed download URLs.
        raise ValueError(f"Meshy API returned HTTP {error.code}. Check key, task ID and credits.") from None
    except urllib.error.URLError:
        raise ValueError("Meshy API connection failed. Try again later.") from None


def outputs(task):
    found = {}
    def walk(obj, prefix=""):
        if not isinstance(obj, dict):
            return
        for name, value in obj.items():
            label = prefix + name
            if isinstance(value, dict):
                walk(value, label + "_")
            elif isinstance(value, str) and (name == "glb" or name.endswith("glb_url")):
                url = urllib.parse.urlsplit(value)
                if url.scheme != "https" or not url.hostname or url.username or url.password:
                    raise ValueError("Meshy returned an invalid asset URL.")
                if not re.fullmatch(r"[A-Za-z0-9_-]+", label):
                    raise ValueError("Meshy returned an invalid asset name.")
                found[label + ".glb"] = value
    walk(task.get("model_urls"), "model_")
    walk(task.get("result"), "result_")
    if not found:
        raise ValueError("This task has no GLB output. Download its export from Meshy instead.")
    return found


def validate_glb(data):
    if len(data) < 20:
        raise ValueError("Downloaded asset is not a GLB.")
    magic, version, length = struct.unpack("<4sII", data[:12])
    if magic != b"glTF" or version != 2 or length != len(data):
        raise ValueError("Downloaded asset is not a complete GLB 2.0 file.")


def stage(task, directory):
    directory.mkdir(parents=True, exist_ok=True)
    files = []
    for name, url in outputs(task).items():
        # Asset requests intentionally have no API Authorization header.
        try:
            with urllib.request.urlopen(url, timeout=120) as response:
                data = response.read(100 * 1024 * 1024 + 1)
        except (urllib.error.HTTPError, urllib.error.URLError):
            raise ValueError("Asset download failed. Retrieve the task again for fresh URLs.") from None
        if len(data) > 100 * 1024 * 1024:
            raise ValueError("Asset exceeds the 100 MB staging limit.")
        validate_glb(data)
        (directory / name).write_bytes(data)
        files.append(name)
    manifest = {"task_id": task.get("id"), "status": task.get("status"),
                "consumed_credits": task.get("consumed_credits"), "files": files}
    (directory / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Downloaded " + str(len(files)) + " GLB assets for review.")


def main():
    operation = os.environ.get("MESHY_OPERATION", "check")
    family = os.environ.get("MESHY_TASK_TYPE", "text-to-3d")
    endpoint = ENDPOINTS[family]
    directory = Path("meshy-output")
    if operation == "check":
        api(ENDPOINTS["text-to-3d"] + "?page_num=1&page_size=1")
        print("Meshy connection verified. No generation task submitted.")
        return
    if operation == "download":
        ident = task_id(os.environ.get("MESHY_TASK_ID", ""))
    elif operation == "rig":
        if os.environ.get("GITHUB_RUN_ATTEMPT", "1") != "1":
            raise ValueError("Do not rerun paid rigging. Resume with download and its task ID.")
        source = Path(os.environ.get("MESHY_MODEL_PATH", ""))
        if not source.is_file() or source.suffix.lower() != ".glb":
            raise ValueError("Rigging requires a local GLB model.")
        data = source.read_bytes()
        validate_glb(data)
        if len(data) > 10 * 1024 * 1024:
            raise ValueError("Optimize the rigging input below 10 MB first.")
        endpoint = ENDPOINTS["rigging"]
        payload = {"model_url": "data:model/gltf-binary;base64," + base64.b64encode(data).decode(),
                   "height_meters": 1.8}
        ident = task_id(api(endpoint, payload)["result"])
        directory.mkdir(exist_ok=True)
        (directory / "task-id.txt").write_text(ident + "\n")
        print("Created Meshy rigging task: " + ident + ". Resume with download; do not resubmit.")
    elif operation in ("preview", "refine"):
        endpoint = ENDPOINTS["text-to-3d"]
        payload = {"mode": operation, "target_formats": ["glb"]}
        if operation == "preview":
            prompt = os.environ.get("MESHY_PROMPT", "").strip()
            if not 1 <= len(prompt) <= 800:
                raise ValueError("A preview requires a prompt of 1–800 characters.")
            payload.update(prompt=prompt, ai_model="meshy-6", should_remesh=True,
                           topology="triangle", target_polycount=4000, pose_mode="a-pose")
        else:
            source = task_id(os.environ.get("MESHY_TASK_ID", ""))
            preview = api(endpoint + "/" + source)
            if preview.get("status") != "SUCCEEDED" or preview.get("type") != "text-to-3d-preview":
                raise ValueError("Refine requires a successful text-to-3D preview task.")
            payload.update(preview_task_id=source, enable_pbr=False)
        # POST is deliberately never retried: an ambiguous response could spend twice.
        ident = task_id(api(endpoint, payload)["result"])
        directory.mkdir(exist_ok=True)
        (directory / "task-id.txt").write_text(ident + "\n")
        print("Created Meshy task: " + ident + ". Use download to resume; do not resubmit.")
    else:
        raise ValueError("Unknown Meshy operation.")
    deadline = time.monotonic() + 1200
    while True:
        task = api(endpoint + "/" + ident)
        status = task.get("status")
        if status == "SUCCEEDED":
            stage(task, directory)
            return
        if status in ("FAILED", "CANCELED"):
            raise ValueError("Meshy task " + status + ". Inspect it in Meshy.")
        if status not in ("PENDING", "IN_PROGRESS"):
            raise ValueError("Unknown Meshy task status.")
        if time.monotonic() >= deadline:
            raise ValueError("Task is still running. Resume with download and the saved task ID.")
        time.sleep(10)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError) as error:
        print("Meshy: " + str(error))
        raise SystemExit(1)
