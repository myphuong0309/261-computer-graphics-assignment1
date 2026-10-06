# Tutorial 2: The Core Graphics Engine in Depth

This tutorial breaks down the three foundational pillars of our engine: the Application Loop (`main.cpp`), the Virtual Camera (`Camera.cpp`), and GPU Shader Communication (`Shader.cpp`).

## 1. `main.cpp` - Context, State, and the Game Loop

`main.cpp` initializes the environment and runs the continuous rendering loop. 

### Context Initialization
Before we can call any OpenGL function, we must create an "OpenGL Context". We use **GLFW** for this.
```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
```
*   **Core Profile**: Disables legacy OpenGL (like `glBegin`).
*   **MSAA (Multi-Sample Anti-Aliasing)**: We set this to 4. This hardware feature slightly blurs the jagged edges (aliasing) of 3D triangles, making atoms and bonds look perfectly smooth.

### Crucial OpenGL State Flags
After initializing GLEW (which loads the actual OpenGL function pointers from your GPU driver), we enable crucial states:
```cpp
glEnable(GL_DEPTH_TEST);
glEnable(GL_MULTISAMPLE);
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
```
*   `GL_DEPTH_TEST`: Activates the Z-Buffer. Without this, OpenGL draws triangles in the exact order you submit them, meaning a background atom could draw *over* a foreground atom.
*   `GL_CULL_FACE`: An optimization. Every triangle has a "front" and "back" face (determined by the winding order of its vertices, usually Counter-Clockwise). Since spheres and cylinders are closed objects, we can never see the inside. `glCullFace(GL_BACK)` tells the GPU to instantly discard back-facing triangles, cutting vertex processing time in half.

### The Game Loop and Delta Time
```cpp
float curTime = (float)glfwGetTime();
App::dt       = curTime - App::lastTime;
App::lastTime = curTime;
```
We calculate `dt` (Delta Time)—the fraction of a second it took to render the previous frame. 
**Why?** If we tell an electron to rotate by "1 degree per frame", a user with a 144Hz monitor will see the atom spinning much faster than a user with a 60Hz monitor. 
Instead, we multiply our speeds by `dt`. "Rotate 90 degrees *per second*". `90.0f * dt` ensures the animation runs at the exact same physical speed regardless of framerate.

## 2. `Camera.cpp` - The Mathematics of Viewing

In real life, a camera captures light. In computer graphics, a camera is just two math matrices that transform the world to fit on your screen.

### The View Matrix (`glm::lookAt`)
Because the camera doesn't actually exist, to simulate moving a camera forward, we must mathematically move the entire 3D universe backward.
`glm::lookAt` calculates this transformation matrix using three vectors:
1.  **Eye**: Where the camera is in 3D space (`position()`).
2.  **Center**: What the camera is looking at (`target`).
3.  **Up**: Which way is "up" (`glm::vec3(0, 1, 0)`).

Our camera is an **Arcball / Orbit Camera**. Instead of moving freely like an FPS game, it revolves around a target. We store this as Spherical Coordinates: `yaw`, `pitch`, and `distance`.
```cpp
float yawR   = glm::radians(yaw);
float pitchR = glm::radians(pitch);
float x = distance * std::cos(pitchR) * std::cos(yawR);
float y = distance * std::sin(pitchR);
float z = distance * std::cos(pitchR) * std::sin(yawR);
```
This math converts Spherical Coordinates (angles and distance) into Cartesian Coordinates (X, Y, Z offsets relative to the target).

### The Projection Matrix (`glm::perspective`)
This matrix creates 3D perspective by defining a "Frustum" (a pyramid with the top chopped off). 
*   **FOV (Field of View)**: How wide the lens is (e.g., 45 degrees).
*   **Aspect Ratio**: Screen width / height. Prevents the image from stretching when you resize the window.
*   **Near & Far Planes**: Any object closer than `near_` or further than `far_` is mathematically clipped (discarded).

## 3. `Shader.cpp` - Bridging CPU and GPU

To get our GLSL code onto the GPU, `Shader.cpp` goes through a strict compilation process similar to C++:
1.  **Create**: `glCreateShader(GL_VERTEX_SHADER)` allocates a shader object.
2.  **Source**: `glShaderSource` feeds the raw GLSL text string into the object.
3.  **Compile**: `glCompileShader` compiles the text into GPU machine code.
4.  **Error Checking**: We use `glGetShaderiv(s, GL_COMPILE_STATUS, &ok)` to check for syntax errors in the GLSL code. If it fails, `glGetShaderInfoLog` fetches the exact error message.
5.  **Linking**: We attach both the Vertex and Fragment shaders to a Program Object (`glAttachShader`), and link them (`glLinkProgram`). This connects the `out` variables of the Vertex shader to the `in` variables of the Fragment shader.

### Uniforms
To pass data (like matrices and light colors) from C++ to the active GLSL shader, we use **Uniforms**.
```cpp
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
```
*   `glGetUniformLocation` queries the GPU for the memory address of the variable named `name`.
*   `glUniformMatrix4fv` uploads an array of 16 floats (a 4x4 matrix) to that address. `glm::value_ptr` extracts the raw float pointer from the GLM C++ object.

In **Tutorial 3**, we will look at how we construct the actual 3D vertices that the Shaders will render.
