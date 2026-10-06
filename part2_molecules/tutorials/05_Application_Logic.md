# Tutorial 5: Application Logic (The Chemistry)

With a robust 3D engine in place, we can construct complex, animated structures using pure math.

## 1. `AtomData.cpp`: The Database
`AtomData.cpp` is our Model data. It maps a string `"C"` to a struct containing:
*   `color = {0.2f, 0.2f, 0.2f}` (Dark Grey, standard CPK color for Carbon).
*   `shells = {2, 4}` (Carbon has 6 total electrons: 2 in the inner shell, 4 in the outer shell).

## 2. `BohrModel.cpp`: Sub-classes and Orbital Planes
To make the atoms animate, we create a subclass of `SceneNode` called `OrbitNode`.
```cpp
class OrbitNode : public SceneNode {
    void update(float dt) override {
        rotAngle = std::fmod(rotAngle + orbitSpeed * dt, 360.0f);
        rotAxis  = orbitAxis;
    }
}
```
Because `BohrModel` calls `node->update(dt)` recursively on the tree every frame, the `OrbitNode` automatically increments its own rotation angle based on elapsed time.

### The Tilt Pivot
If all electrons orbited in the XY plane, the atom would look flat. We use a `tiltPivot` node to tilt each shell's orbit.
We attach the Torus ring to the `tiltPivot`. We *also* attach the `OrbitNode` to the `tiltPivot`.
```cpp
orbitPivot->orbitAxis = glm::vec3(0.0f, 0.0f, 1.0f); // Local Z-Axis
```
Because both the Torus and the `OrbitNode` are children of the `tiltPivot`, they share the exact same local coordinate space. The Torus mesh is naturally flat on the XY plane. Therefore, if we tell the `OrbitNode` to rotate around the Z-axis (which is perpendicular to XY), the electron will perfectly trace the path of the Torus!

## 3. `MoleculeScene.cpp`: The Vector Math of Bonds
This is the most mathematically intense part of the project. How do we take a Cylinder (which naturally points straight up along the Y-axis) and stretch it so it connects Atom A to Atom B?

### Step A: The Direction Vector
First, we find the mathematical vector pointing from A to B.
```cpp
glm::vec3 dir = pb - pa;
float len = glm::length(dir);
glm::vec3 norm = glm::normalize(dir);
```

### Step B: The Rotation Axis (Cross Product)
Our cylinder points UP `(0, 1, 0)`. We want it to point towards `norm`. 
To rotate one vector to match another, we need to know the **Axis of Rotation**. 
In 3D math, the **Cross Product** of two vectors generates a third vector that is perfectly perpendicular to both of them. This perpendicular vector is the exact axis we need to rotate around!
```cpp
glm::vec3 up(0, 1, 0);
glm::vec3 axis = glm::cross(up, norm);
float sinA = glm::length(axis);
```

### Step C: The Angle (Dot Product)
How *much* do we rotate around that axis? We use the **Dot Product**.
The Dot Product of two normalized vectors gives the Cosine of the angle between them.
```cpp
float cosA = glm::dot(up, norm);
glm::mat4 R = glm::rotate(glm::mat4(1.0f), std::atan2(sinA, cosA), glm::normalize(axis));
```
We use `std::atan2(sinA, cosA)` because it safely calculates the angle even if the vectors are pointing in weird directions.

### Step D: Scaling and Positioning
Finally, we apply our TRS (Translate, Rotate, Scale) logic.
1. **Scale**: We scale the cylinder's Y-axis by `len` (the distance between atoms). We scale X and Z by a tiny amount (`bondR`) to make it a thin stick.
2. **Rotate**: We apply our calculated matrix `R`.
3. **Translate**: We move the cylinder to the exact midpoint between Atom A and Atom B. `mid = (pa + pb) * 0.5f;`.

This perfect synergy of Cross Products, Dot Products, and Matrices is what allows us to dynamically render any molecule geometry instantly!

In our final **Tutorial 6**, we will look at how the GPU calculates light hitting these cylinders and spheres.
