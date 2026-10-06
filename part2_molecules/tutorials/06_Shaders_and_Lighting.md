# Tutorial 6: Shaders and the Mathematics of Light

To make a 3D sphere look like a physical object, we must simulate light. Real light consists of trillions of photons bouncing endlessly (Global Illumination). In real-time rendering, this is too slow. Instead, we use the **Phong Reflection Model**, a mathematical hack that looks highly realistic.

This math runs entirely on the GPU inside `phong.frag`.

## 1. Vector Normalization
Before we do lighting math, we must ensure our directional vectors are **Normalized**. A normalized vector has a length of exactly 1.0. If a vector has a length of 2.0, our light math will result in a color that is 200% too bright.
```glsl
vec3 norm     = normalize(Normal);
vec3 lightDir = normalize(light.position - FragPos);
vec3 viewDir  = normalize(viewPos - FragPos);
```

## 2. The Ambient Component
Even parts of an atom in total shadow shouldn't be pitch black, because in reality, light bounces off the environment. We fake this using Ambient light.
```glsl
vec3 ambient = light.ambient * material.ambient;
```
If the light ambient color is grey `(0.2, 0.2, 0.2)` and the atom is red `(1.0, 0.0, 0.0)`, the result is dark red `(0.2, 0.0, 0.0)`.

## 3. The Diffuse Component (Lambertian Reflectance)
This is the core lighting. If a flashlight hits a wall directly at a 90-degree angle, it is bright. If it hits at a grazing 10-degree angle, it is dim.

We calculate this using the **Dot Product** between the surface normal (`norm`) and the direction to the light (`lightDir`). 
Mathematically, the dot product of two normalized vectors equals the cosine of the angle between them. 
*   If the angle is 0° (pointing exactly at the light), cosine is 1.0 (Maximum brightness).
*   If the angle is 90° (light skimming the edge), cosine is 0.0 (No brightness).
*   If the angle is 180° (light hitting the back), cosine is -1.0. We use `max(..., 0.0)` to ensure colors don't go negative.

```glsl
float diff   = max(dot(norm, lightDir), 0.0);
vec3 diffuse = light.diffuse * (diff * material.diffuse);
```

## 4. The Specular Component (The Shiny Highlight)
Shiny materials like plastic or metal have a concentrated white highlight. This happens when the light bounces off the surface and hits the camera (your eye) almost perfectly.

1.  **Reflect**: We calculate the exact direction the light bounces off the surface using GLSL's built-in `reflect` function.
2.  **Dot Product**: We compare that bounced vector (`reflectDir`) with the camera direction (`viewDir`). If they align, the dot product is close to 1.0.
3.  **Shininess (Power)**: If the dot product is 0.9, the highlight would be huge and blurry. By taking $0.9^{32}$ (using the `pow` function), the number shrinks rapidly to ~0.03. But $0.99^{32}$ remains high at ~0.72. This mathematical power function exponentially tightens the highlight into a tiny, sharp dot!

```glsl
vec3 reflectDir = reflect(-lightDir, norm);
float spec      = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
vec3 specular   = light.specular * (spec * material.specular);
```
*(Note: we use `-lightDir` because the GLSL `reflect` function expects the incoming light vector to point FROM the light TO the surface, whereas our `lightDir` points FROM the surface TO the light).*

## 5. Outputting to the Screen
Finally, we add the three components together.
```glsl
vec3 result = ambient + diffuse + specular;
FragColor   = vec4(result, 1.0);
```
The result is sent to the framebuffer as a `vec4` (Red, Green, Blue, Alpha). Because our objects are solid, Alpha is `1.0`.

---
**Congratulations!** You have reached the end of the tutorials. 
You now understand how `main.cpp` initializes the GPU State Machine, how `SceneNode` recursively multiplies matrices to build complex hierarchies, how `MoleculeScene` uses Cross Products to align bonds, and how `phong.frag` uses Dot Products to simulate light. 

You are no longer a beginner in Computer Graphics!
