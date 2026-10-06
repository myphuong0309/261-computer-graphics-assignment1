# Tutorial 4: The Scene Graph and Matrix Math

A common rookie mistake in graphics programming is trying to calculate the absolute 3D world position of every single object manually. If you have an atom rotating, and an electron orbiting that atom, the trigonometry required to calculate the electron's position from scratch every frame is a nightmare.

Instead, we use a **Scene Graph**: a tree data structure of `SceneNode`s.

## 1. Local Space and Transformations
Every `SceneNode` defines a position, rotation, and scale. These are **Local** coordinates.
If an electron node has a position of `(radius, 0, 0)`, it simply means "I am located `radius` units to the right of my parent".

To convert these local coordinates into a 4x4 Transformation Matrix, we look at `SceneNode::localTransform()`:
```cpp
glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(rotAngle), rotAxis);
glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
return T * R * S;
```
### The Order of Multiplication Matters!
In matrix math, `A * B` is not the same as `B * A`. 
We multiply in the order **T * R * S** (Translation * Rotation * Scale). Because OpenGL vectors are treated as column vectors, transformations are applied right-to-left.
1.  **Scale First**: The raw mesh is scaled around its own local origin.
2.  **Rotate Second**: The scaled mesh is rotated around its local origin.
3.  **Translate Last**: The rotated, scaled mesh is moved to its final position.

If you translated first and then rotated, the object would orbit the origin like a planet, rather than spinning in place like a top!

## 2. Recursive World Traversal
How do we figure out where the electron is in the absolute "World Space"?
In `SceneNode::draw`, you see this crucial line:
```cpp
glm::mat4 world = parentTransform * localTransform();
```
And then, for all of its children:
```cpp
for (const auto& child : children)
    child->draw(shader, world);
```
This is a recursive depth-first traversal of the tree.
1. The `root` is given an Identity Matrix (a matrix that does nothing) as its `parentTransform`. Its `world` matrix is just its `localTransform`.
2. It passes this `world` matrix down to the `tiltPivot`.
3. The `tiltPivot` multiplies its own local tilt by the root's matrix.
4. The `OrbitNode` multiplies its own local rotation by the `tiltPivot`'s matrix.
5. Finally, the electron sphere receives a `parentTransform` that *already includes* the tilt and the orbit. It just applies its `localTransform` (the scale and radius offset).

Through simple recursion, the complex math of compound orbiting objects is solved automatically!

## 3. The Normal Matrix
Before calling `mesh->draw()`, we upload the `world` matrix to the shader. But we also upload something called a `normalMatrix`.
```cpp
glm::mat3 nm = glm::mat3(glm::transpose(glm::inverse(world)));
shader.setMat3("normalMatrix", nm);
```
Why can't we just use the `world` matrix to transform the Normals (the arrows pointing away from the surface)? 

If we **Non-Uniformly Scale** an object (e.g., squishing a sphere into an oval by scaling X by 2 and Y by 1), the angles of the surface change. If we simply apply that same scale to the Normal vector, the Normal will lean the *wrong way* and will no longer be perpendicular to the surface. Lighting will be completely ruined.

To fix this mathematical anomaly, we must use the **Transpose of the Inverse** of the world matrix to transform Normals. This mathematically preserves the perpendicular angle regardless of how we squish or skew the node!

In **Tutorial 5**, we will see how we use this Scene Graph to build actual molecules.
