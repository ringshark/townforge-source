# 2D mobile startup repair — 2026-10-23

A phone black-screen report exposed two inherited startup costs: the 2D build
reserved a fixed 512 MB WASM heap and `WarmWildernessCache` still loaded 3D
models, generated four 2048px terrain images and built scatter while idle in
2D town. A live Chrome trace confirmed those 3D glTF loads in the published
2D game. The old startup heap measured 536870912 bytes.

2D now starts at 128 MB, can grow to the existing 512 MB ceiling, and skips
3D idle preparation. 3D/combined build heap settings remain unchanged. The
same assets and save location remain in use. Startup status becomes visible
again for later errors; aborts and core script failures show a readable message.
Audio resumes handle denied/unavailable devices without unhandled rejections,
including the engine's resume calls. The AudioContext wrapper also accepts
the prefixed WebKit constructor and tolerates an absent audio constructor.

Validation: 2D Emscripten/raylib build; transition tests prove no 3D cache work
in classic view and preserve the one-time 3D path; variant/backup checks;
landscape input and shell tests; Chromium and WebKit browser startup, movement,
menu interaction, day/night and visible error fallback. WebKit checks permit
service workers and reload the phone context. The revised Chrome startup heap
measures 134217728 bytes in all three scenarios and no 3D models load.
Physical iPhone verification still needs the player's device.
