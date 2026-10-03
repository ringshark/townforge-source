"""Extract and fit the approved Meshy batch to existing armor attachment frames.

Usage: python tools/characters3d/fit_armor.py RAW_DIRECTORY OUTPUT_DIRECTORY
Requires numpy and trimesh. Generates no new Meshy requests.
"""
import json
from pathlib import Path
import sys
import numpy as np
import trimesh

TASKS = {"chest": "01a0ff8b-1cf6-7375-9fd5-bd483fa8fd34",
         "shoulder": "01a0ff8b-2926-7708-b061-9729ec903007",
         "greave": "01a0ff8b-3174-7409-8053-ea0acf5384b6"}

def clip(mesh, axis, boundary, positive):
    origin = np.zeros(3); origin[axis] = boundary
    normal = np.zeros(3); normal[axis] = 1 if positive else -1
    return trimesh.intersections.slice_mesh_plane(mesh, normal, origin, cap=False)

def extract(mesh, name):
    if name == "chest":
        # Remove the display stand, long thigh tassets, gorget and shoulder caps.
        for axis, value, positive in [(1, -.10, True), (1, .72, False),
                                      (0, -.30, True), (0, .30, False)]:
            mesh = clip(mesh, axis, value, positive)
    elif name == "shoulder":
        # The generated figure carries its layered pauldron on +X.
        for axis, value, positive in [(0, .13, True), (1, .43, True), (1, .75, False)]:
            mesh = clip(mesh, axis, value, positive)
        # Straighten the figure's tilted upper arm before fitting to a bone.
        mesh.vertices[:, 0] -= (.75 - mesh.vertices[:, 1]) * .18
    else:
        for axis, value, positive in [(0, -.065, False), (1, -.825, True), (1, -.365, False)]:
            mesh = clip(mesh, axis, value, positive)
        # Straighten its stance; armor follows animation rather than a baked pose.
        mesh.vertices[:, 0] += (-.365 - mesh.vertices[:, 1]) * .25
    mesh.merge_vertices(); mesh.remove_unreferenced_vertices()
    # Remove stray tiny fragments left by cropping a remeshed preview.
    parts = mesh.split(only_watertight=False)
    # Each useful piece is one continuous shell. Detached fragments belong to
    # the surrounding figure, and would also skew the normalization bounds.
    mesh = max(parts, key=lambda part: len(part.faces))
    if name != "chest":
        # Bone-space +Y follows the limb downward; preserve +Z facing forward.
        mesh.apply_transform(np.diag([-1.0, -1.0, 1.0, 1.0]))
    return mesh

def fit(mesh, name):
    targets = {"chest": ((-.135, -.25, -.11), (.135, .13, .10)),
               "shoulder": ((-.084, -.045, -.084), (.084, .155, .084)),
               "greave": ((-.062, .015, -.058), (.062, .355, .075))}
    low, high = map(np.array, targets[name]); src_low, src_high = mesh.bounds
    size = src_high - src_low
    if np.any(size <= .001):
        raise ValueError("Armor crop is empty or degenerate")
    mesh.vertices = (mesh.vertices - src_low) * ((high - low) / size) + low
    mesh.visual = trimesh.visual.ColorVisuals(mesh=mesh, vertex_colors=np.tile([240,240,240,255], (len(mesh.vertices),1)))
    mesh.fix_normals(multibody=True)
    return mesh

def main(raw, output):
    raw = Path(raw); output = Path(output); output.mkdir(parents=True, exist_ok=True)
    ledger = json.loads((raw / "tasks.json").read_text())
    assert sum(task["consumed_credits"] for task in ledger.values()) == 60
    manifest = {"batch": "townforge-armor-20261003", "credits": 60, "pieces": {}}
    for name, ident in TASKS.items():
        assert ledger[name]["task_id"] == ident and ledger[name]["status"] == "SUCCEEDED"
        source = raw / name / "model_glb.glb"
        mesh = trimesh.load(source, force="scene").to_mesh()
        original_faces = len(mesh.faces)
        mesh = fit(extract(mesh, name), name)
        stem = {"chest": "steel-cuirass", "shoulder": "steel-pauldron-r", "greave": "steel-greave-r"}[name]
        files = [stem + ".glb"]
        mesh.export(output / files[0], include_normals=True)
        if name != "chest":
            mirrored = mesh.copy()
            mirrored.apply_transform(np.diag([-1.0, 1.0, 1.0, 1.0]))
            files.append(stem[:-1] + "l.glb")
            mirrored.export(output / files[1], include_normals=True)
        manifest["pieces"][name] = {"task_id": ident, "source_triangles": original_faces,
            "triangles": len(mesh.faces), "bounds": mesh.bounds.tolist(), "files": files}
        print(name, len(mesh.faces), "triangles", files)
    (output / "meshy-armor.json").write_text(json.dumps(manifest, indent=2) + "\n")

if __name__ == "__main__":
    main(*sys.argv[1:])
