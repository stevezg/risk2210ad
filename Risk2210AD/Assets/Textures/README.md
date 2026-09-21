# Textures

High-resolution map textures are optional. The board renders procedurally (vector landforms in
`Scripts/View/VectorLandforms.cs`) so the game runs with an empty folder.

To use sliced artwork from the physical board, place sprites under
`Assets/Resources/Textures/Territories/<Territory Name>.png` (exact territory name, e.g.
`Enclave of the Bear.png`, `Amazon Desert.png`). `BoardView` picks them up automatically and draws
each sprite centred on its territory beneath the neon border polygon. Recommended import settings:
Sprite (2D and UI), pixels-per-unit tuned so one territory spans roughly 1.5-3 world units,
mipmaps on, compression high quality.

Slicing pipeline (one-off, outside Unity): scan or photograph the board at 300+ dpi, warp it
onto the layout in `MapGraph.CreateStandard` (positions are in 1/50th of the reference pixel
grid), and cut one PNG per territory with transparent surroundings.
