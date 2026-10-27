# Computer Graphics - Assignment 1 (HCMUT, Semester I, 2026-2027)

2D / 3D object rendering with modern OpenGL (C++17, OpenGL 3.3 core profile, GLFW, GLEW, Dear ImGui, GLM).

| Folder | Content |
|---|---|
| [part1_shapes/](part1_shapes/) | **Part 1** - drawing basic 2D / 3D shapes, 5 render modes, transformations, `.obj` / `.ply` import |
| [part2_molecules/](part2_molecules/) | **Part 2** - Bohr atom model and ball-and-stick molecules |
| [common/](common/) | code shared by both parts: `Shader`, `Camera`, Dear ImGui, GLM |
| [docs/](docs/) | assignment statement (`cgAssignment1.pdf`) |
| [report/](report/) | report (`report.tex` + `report.pdf`) and its images |

---

## Setup (Ubuntu / Debian)

```bash
sudo apt install build-essential libglfw3-dev libglew-dev
```

Dear ImGui and GLM are bundled in `common/vendor/`; the stb headers (Part 1 only) are bundled in `part1_shapes/vendor/stb/`.
No other dependency is needed.

## Build and run

```bash
make            # build both parts            (make part1 / make part2 for one part)
make run1       # run Part 1                  (make run2: Part 2)
make test       # unit tests of both parts (no GPU needed)
make clean      # remove the build output of both parts
```

Each part also builds on its own (`cd part1_shapes && make -j4`, then `make run` or `./build/shapes`;
`cd part2_molecules && make -j4`, then `make run` or `./build/molecules`).
The programs must be started from their own folder so that `shaders/` (and Part 1's `assets/`) are found - `make run1` / `make run2` do this.

---

# Part 1: Drawing Basic Shapes

Interactive application that draws 2D and 3D shapes with a Dear ImGui interface, mouse / keyboard camera control,
translate / rotate / scale editing and five render modes.

## Features (assignment checklist)

| Requirement | Where |
|---|---|
| 2D shapes: triangle, rectangle, pentagon, regular hexagon, circle, ellipse, trapezoid, star, arrow | *Add Shape ▸ 2D shapes* |
| 3D solids: cube, sphere, cylinder, cone, truncated cone, tetrahedron, torus, prism | *Add Shape ▸ 3D shapes* |
| Surface `z = f(x, y)` from a user-typed formula (presets, ranges, resolution) | *Add Shape ▸ Surface…*, edit in Inspector ▸ Shape |
| Import `.obj` / `.ply` models (ascii + binary PLY, normals/UV/colors optional) | *File ▸ Import model…* |
| GUI with menus / toolbars to choose the shape to add | menu bar + Inspector panel |
| Zoom / pan / rotate with mouse and keyboard | see Controls |
| Translate / rotate / scale of each object (+ live model matrix) | Inspector ▸ Transform |
| Flat color | render mode *Flat color* |
| Gouraud vertex-color interpolation | render mode *Gouraud* (per-vertex lighting of per-vertex colors; rainbow or 2-color gradient) |
| Phong shading (per-fragment lighting) | render mode *Phong* |
| Texture mapping with a user image | render mode *Texture* (png/jpg/bmp/tga; pick from `assets/textures` or type a path) |
| Wireframe (edges only) | render mode *Wireframe* - real edges, not the triangulation |

Extras: several objects per scene (select / duplicate / delete), yellow selection box, ground grid + axes,
configurable light, screenshot (`F12`), demo scene (*Scene ▸ Load demo scene*).

## Controls

| Input | Action |
|---|---|
| Left drag | orbit camera |
| Right drag | pan |
| Mouse wheel | zoom |
| **Ctrl** + left drag | rotate the selected object |
| Arrow keys / **Shift** + arrows | orbit / pan |
| `+` `-` or PageUp/PageDown | zoom |
| `R` | reset camera |
| `W` | toggle wireframe on the selected object (all objects if none is selected) |
| `Del` | delete selected object |
| `F12` | save a screenshot (`screenshot_<time>.png`, scene only) |
| `H` | help window |
| `Esc` | quit |

*View ▸ Front view (2D)* looks straight at the XY plane, where the 2D shapes live.

### Surface formulas
Variables `x`, `y`; operators `+ - * / ^`; constants `pi`, `e`; functions
`sin cos tan asin acos atan sinh cosh tanh sqrt abs exp log ln floor ceil pow(a,b) atan2(a,b) min(a,b) max(a,b)`.
Implicit multiplication works (`2x`, `3sin(x)`). A leading `z =` is ignored. Press **Enter** to apply.

## Architecture

```
part1_shapes/
├── src/
│   ├── main.cpp         window, GLFW callbacks, render loop, CLI options
│   ├── App.h            shared state (scene, camera, light, view presets)
│   ├── Gui.h/cpp        menus, inspector, import dialog (ImGui)
│   ├── Scene.h/cpp      object list, layout, grid/axes, selection box, demo scene
│   ├── SceneObject.h/cpp  one shape: geometry + transform + appearance
│   ├── Geometry.h/cpp   CPU mesh generators for every shape + mesh utilities (no OpenGL)
│   ├── ShapeMesh.h/cpp  VAO/VBO/EBO wrapper (triangles + wireframe edge list)
│   ├── Surface.h/cpp    z = f(x, y) mesh        ExprParser.h/cpp   formula compiler
│   ├── ModelLoader.h/cpp  .obj / .ply importers
│   ├── Texture.h/cpp    stb_image loader + cache  Paths.h/cpp  asset lookup
│   └── Screenshot.h/cpp
├── shaders/shapes.vert, shapes.frag    one program, `mode` uniform selects the render mode
├── assets/textures, assets/models      sample images / models
├── vendor/stb/                         stb_image.h, stb_image_write.h
├── tests/test_core.cpp                 `make test`
└── Makefile
```

Shader modes: `0` flat, `1` Gouraud (Phong lighting evaluated in the vertex shader from the vertex color and
interpolated), `2` Phong (per fragment), `3` texture × Phong, `4` wireframe color, `5` unlit vertex color (grid/axes).
Lighting is two-sided so 2D shapes are lit from either face.

Model matrix: `M = T · Rz · Ry · Rx · S` (shown live in Inspector ▸ Transform ▸ Model matrix).

Part 1 uses `Shader` and `Camera` from `common/`; the sphere / cylinder / torus generation loops and the Phong shader were
adapted from Part 2 (Part 2's `Mesh` stores only position + normal, so it cannot carry the UVs / vertex colors needed here).

## Command-line options (for screenshots / testing)

```
./build/shapes --demo                         # all shapes
./build/shapes --add star --add torus --mode gouraud
./build/shapes --surface "sin(sqrt(x^2+y^2))" --model assets/models/torusknot.obj
./build/shapes --demo --select 9 --texture assets/textures/uvgrid.png --with-gui --screenshot out.png
# --mode flat|gouraud|phong|texture|wire   --view front|top   --distance d   --select i|-1
# --no-grid   --light x,y,z   --frames N
```

---

# Part 2: Visualization of Atoms and Molecules

Interactive application: Bohr atomic model with orbiting electrons, and a ball-and-stick molecule viewer
with an editor for atom types / positions.

## Features

| Feature | Where |
|---|---|
| GUI with menus/toolbars (Dear ImGui) | control panel on the left |
| Bohr atom model - electrons in shells (23 elements, H-Ca, Fe, Cu, Zn) | *View Mode ▸ Bohr Atom Model* |
| Ball-and-stick molecules (H₂O, CO₂, NH₃, CH₄, O₂, N₂, HCl, NaCl): CPK-colored spheres, cylinder bonds, single / double / triple | *View Mode ▸ Ball-and-Stick* |
| Configuration of atom types and positions, bonds and bond orders, automatic bond detection | *Edit atoms & bonds* |
| Bond lengths and bond angles of the current molecule | *Geometry* |
| Animated electron motion along the orbits | *Animate Electrons*, *Speed* |
| Molecular vibration (symmetric stretch about the centre of mass, bonds follow the atoms) and rotation | *Molecular Vibration*, *Molecular Rotation* |
| Phong shading with configurable light, wireframe | *Lighting*, *Wireframe (W)* |
| Orbit / pan / zoom camera | mouse, *Camera* |
| Screenshots | `F12` (3D view), `Shift+F12` (whole window), *Screenshot* button, `--screenshot` |

## Controls

| Input | Action |
|---|---|
| Left mouse drag | rotate view |
| Right mouse drag | pan view |
| Scroll wheel | zoom in/out |
| `R` | reset camera |
| `W` | toggle wireframe |
| `F12` / `Shift+F12` | screenshot of the 3D view / of the whole window (`screenshot_<time>.png`) |
| `Esc` | quit |

## Command-line options (for screenshots / testing)

```
./build/molecules --element Na                                  # Bohr model of sodium
./build/molecules --molecule CO2 --rotate                       # molecule view
./build/molecules --molecule H2O --with-gui --editor --screenshot out.png
./build/molecules --molecule HCl --amplitude 0.2 --time 0.0866 --screenshot stretched.png
# --wireframe  --paused  --no-vibrate  --speed x  --time t  --yaw deg  --pitch deg  --distance d
# --light x,y,z  --frames N   (with --screenshot the animation is frozen at --time t)
```

## Architecture

```
part2_molecules/
├── src/
│   ├── main.cpp          application entry, render loop, GUI, molecule editor, CLI
│   ├── Mesh.h/cpp        procedural meshes (sphere, cylinder, torus): CPU data + GPU upload
│   ├── SceneNode.h/cpp   hierarchical scene graph
│   ├── AtomData.h/cpp    periodic table (mass, covalent radius, CPK color, Bohr shells), molecules, bond detection
│   ├── BohrModel.h/cpp   Bohr atom scene builder + electron animation
│   ├── MoleculeScene.h/cpp  ball-and-stick scene + vibration/rotation
│   └── Screenshot.h/cpp  PNG screenshot (stb_image_write)
├── shaders/phong.vert, phong.frag
├── tests/test_part2.cpp  `make test` (no GPU needed)
├── vendor/stb/           stb_image_write.h
├── tutorials/            step-by-step notes on the code
└── Makefile
```

### Scene graph

```
SceneNode (root)                                  MolRotNode (root, rotation about Y through the centre of mass)
├── SceneNode (nucleus sphere)                    ├── VibrationNode (atom 1) → SceneNode (sphere)
├── ShellPivot (shell k, tilted about X)          ├── VibrationNode (atom 2) → SceneNode (sphere)
│   ├── SceneNode (torus ring of radius r_k)      ...
│   └── OrbitNode (one per electron, rotating)    └── SceneNode (bond cylinder, re-placed every frame)
│       └── SceneNode (electron sphere)
└── ...
```

The sphere and the cylinder are shared by every node; each Bohr shell has its own torus (constant tube radius, so
the electrons are never hidden inside a ring). MSAA 4× is enabled and shading is per-fragment Phong with a configurable point light.

## Shared code (`common/`)

`common/src` (`Shader`, `Camera`) and `common/vendor` (`imgui`, `glm`) are compiled by both Makefiles
(into each part's own `build/` folder), so the parts stay independent programs but both need `common/` next to them.
`Camera` is an orbit camera (yaw / pitch / distance / target) with left-drag orbit, right-drag pan and wheel zoom.

## Report

`report/report.tex` (and the compiled `report/report.pdf`) describes the architecture, features and usage; build it with
`cd report && latexmk -lualatex report.tex` (LuaLaTeX is needed for the Vietnamese names on the title page; with pdfLaTeX the `vntex` package is used instead). Images used in the report (logo and screenshots) are in `report/images/`.
