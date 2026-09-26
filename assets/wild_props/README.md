# Wilderness props - KayKit Medieval Hexagon Pack 1.0 (CC0 1.0 Universal)

Created/distributed by Kay Lousberg (www.kaylousberg.com).
Source: https://github.com/KayKit-Game-Assets/KayKit-Medieval-Hexagon-Pack-1.0
License: CC0 1.0 Universal (public domain) - free for personal, educational and
commercial use, no attribution required (credit appreciated).

A subset of the pack's `Assets/gltf/decoration/{nature,props}` models (glTF +
.bin). Their `images`/`textures`/`samplers` entries and material
`baseColorTexture` references were stripped (UVs kept): all of them use the one
`hexagons_medieval.png` palette texture, which the game loads once and assigns
to every material at runtime instead of decoding a copy per model. Used by the 3D wilderness dressing (see
Wild3DBuildDressing in main.cpp): tree clusters, rocks and stumps inside the
map, roadside camps, and a horizon ring of mountains/hills outside the edges.
