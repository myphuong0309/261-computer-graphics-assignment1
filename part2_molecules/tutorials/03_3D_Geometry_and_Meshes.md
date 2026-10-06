# Tutorial 3: 3D Geometry and Memory Layouts

In modern OpenGL, there are no built-in functions to draw a sphere or a cylinder. We must generate the raw math points ourselves and send them to the GPU memory.

## 1. The Vertex Struct and Memory Layout
In `Mesh.h`, we define a single vertex:
```cpp
struct Vertex {
    glm::vec3 position; // 12 bytes (3 floats)
    glm::vec3 normal;   // 12 bytes (3 floats)
};
```
Because of how C++ structs work, this is a "tightly packed" structure. Each `Vertex` takes up exactly 24 bytes. An array of these vertices in RAM is just one contiguous block of memory `[Pos, Norm, Pos, Norm, Pos, Norm...]`. This is called an **Interleaved Layout**, and GPUs love it because it is cache-friendly.

## 2. GPU Buffers: VAO, VBO, and EBO
In the `Mesh` constructor, we upload this RAM array to VRAM (Video RAM).

### VBO (Vertex Buffer Object)
```cpp
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
```
This reserves raw memory on the GPU and copies our byte array into it. `GL_STATIC_DRAW` is a hint telling the GPU driver, "I am going to upload this once and draw it millions of times without changing the data."

### EBO (Element Buffer Object)
The EBO stores our Indices. Instead of storing 3 vertices for every single triangle (which would duplicate vertices shared by adjacent triangles), we store the unique vertices in the VBO, and the EBO just stores integers saying "Triangle 1 is made of Vertex 0, 5, and 6".

### VAO (Vertex Array Object)
The GPU now has a VBO full of raw bytes, but it has no idea what those bytes mean. The VAO acts as a "Memory Map" or "State Record".
```cpp
glEnableVertexAttribArray(0); // Attribute 0: Position
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

glEnableVertexAttribArray(1); // Attribute 1: Normal
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
```
*   `sizeof(Vertex)` is the **Stride**: How many bytes to jump to get to the *next* vertex (24 bytes).
*   `offsetof` is the **Offset**: Where does this specific attribute start within the 24-byte block? Position starts at byte 0. Normal starts at byte 12.

When we call `mesh->draw()`, we simply bind the VAO and call `glDrawElements`. The GPU instantly knows exactly how to read the VBO.

## 3. Procedural Generation Mathematics
Instead of loading a `.obj` file from Maya or Blender, we generate our shapes procedurally using Trigonometry.

### Generating a Sphere
In `Mesh::makeSphere`, we use spherical coordinates:
*   `sectorAngle` goes around the equator (Longitude, 0 to 2π).
*   `stackAngle` goes from the North Pole to the South Pole (Latitude, π/2 to -π/2).

```cpp
float xy = std::cos(stackAngle);
float z  = std::sin(stackAngle);
float x = xy * std::cos(sectorAngle);
float y = xy * std::sin(sectorAngle);
```
For a unit sphere (radius 1), the mathematical **Normal** vector at any point on the surface is identical to the position vector itself! This makes lighting a procedural sphere incredibly easy.

### Generating a Cylinder
In `Mesh::makeCylinder`, our cylinder is perfectly aligned along the **Y-Axis**.
```cpp
vb.position = glm::vec3(cx, -hHalf, cz);
vt.position = glm::vec3(cx,  hHalf, cz);
```
We generate two vertices at a time: one at the bottom (`-hHalf`) and one at the top (`+hHalf`). The normal for both of these points simply points straight out horizontally: `glm::vec3(std::cos(angle), 0.0f, std::sin(angle))`.

### Winding Order and Cap Indices
When generating the caps for the cylinder, order matters. OpenGL uses **Counter-Clockwise (CCW)** winding by default to define the "front" of a triangle.
If you look at the bottom cap from the outside, the vertices must be defined in CCW order. If they are defined Clockwise, `glCullFace(GL_BACK)` will think you are looking at the inside of the triangle and delete it!

In **Tutorial 4**, we will learn how we take a single sphere mesh and draw it 50 times in different locations to build an atom.
