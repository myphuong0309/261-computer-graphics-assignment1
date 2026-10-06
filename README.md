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
make test       # Part 1 unit tests (no GPU needed)
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

Interactive application: Bohr atomic model with orbiting electrons, and a ball-and-stick molecule viewer.

## Features

| Feature | Status |
|---|---|
| GUI with menus/toolbars (Dear ImGui) | yes |
| Bohr atom model - electrons in shells | yes |
| Simple molecule viewer (H₂O, CO₂, …), atoms as colored spheres (CPK scheme), bonds as cylinders | scene implemented (`MoleculeScene`); see *Known issues* |
| Animated electron motion along orbitals | yes |
| Molecular vibration and rotation animation | implemented in `MoleculeScene` |
| Phong shading with configurable light | yes |
| Configurable camera (orbit / pan / zoom) | yes |
| Scene graph hierarchy | yes |
| 23 elements in the periodic table, 8 preloaded molecules, single / double / triple bonds | yes |

## Controls

| Input | Action |
|---|---|
| Left mouse drag | rotate view |
| Right mouse drag | pan view |
| Scroll wheel | zoom in/out |
| `R` | reset camera |
| `W` | toggle wireframe flag |
| `Esc` | quit |

## Architecture

```
part2_molecules/
├── src/
│   ├── main.cpp          application entry, render loop, GUI
│   ├── Mesh.h/cpp        procedural mesh generation (sphere, cylinder, torus)
│   ├── SceneNode.h/cpp   hierarchical scene graph
│   ├── AtomData.h/cpp    periodic table (CPK colors, radii, Bohr shells) + predefined molecules
│   ├── BohrModel.h/cpp   Bohr atom scene builder + electron animation
│   └── MoleculeScene.h/cpp  ball-and-stick scene + vibration/rotation
├── shaders/phong.vert, phong.frag
├── tutorials/            step-by-step notes on the code
└── Makefile
```

### Scene graph

```
SceneNode (root)                                  MolRotNode (root, optional rotation)
├── SceneNode (nucleus sphere)                    ├── VibrationNode (atom 1) → SceneNode (sphere)
├── OrbitNode (shell 1, rotating)                 ├── VibrationNode (atom 2) → SceneNode (sphere)
│   └── SceneNode (electron sphere)               ...
├── OrbitNode (shell 2, rotating) ...             └── SceneNode (bond cylinder, pre-oriented)
└── SceneNode (torus ring - orbital visualization)
```

All objects share **two meshes** (sphere + cylinder) and only their transforms vary. MSAA 4× is enabled and shading is per-fragment Phong
with a configurable point light.

### Supported molecules
H₂O (water), CO₂ (carbon dioxide), NH₃ (ammonia), CH₄ (methane), O₂ (oxygen), N₂ (nitrogen), HCl (hydrogen chloride), NaCl (sodium chloride).

### Supported elements (Bohr model)
H, He, Li, Be, B, C, N, O, F, Ne, Na, Mg, Al, Si, P, S, Cl, Ar, K, Ca, Fe, Cu, Zn

### Known issues (state of the code at the time of writing)
- In `main.cpp` the radio button that switches to the *Ball-and-Stick Molecule* view is commented out, so the molecule scene cannot currently be opened from the GUI.
- The `W` key and the *Wireframe (W)* checkbox change `App::wireframe`, but that flag is not yet applied to the scene nodes, so it has no visible effect.

---

## Shared code (`common/`)

`common/src` (`Shader`, `Camera`) and `common/vendor` (`imgui`, `glm`) are compiled by both Makefiles
(into each part's own `build/` folder), so the parts stay independent programs but both need `common/` next to them.
`Camera` is an orbit camera (yaw / pitch / distance / target) with left-drag orbit, right-drag pan and wheel zoom.

## Report

`report/report.tex` (and the compiled `report/report.pdf`) describes the architecture, features and usage; build it with
`cd report && latexmk -lualatex report.tex` (LuaLaTeX is needed for the Vietnamese names on the title page; with pdfLaTeX the `vntex` package is used instead). Images used in the report (logo and screenshots) are in `report/images/`.
