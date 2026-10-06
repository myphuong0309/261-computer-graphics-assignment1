#version 330 core
// Part 1 vertex shader. Based on part2_molecules/shaders/phong.vert, extended with
// vertex color + texture coordinates and the per-vertex lighting used by Gouraud shading.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aUV;

out vec3 FragPos;
out vec3 Normal;
out vec3 VertexColor;
out vec2 UV;
out vec3 GouraudColor;   // lit color computed here (mode 1) and interpolated across the triangle

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;
uniform mat3  normalMatrix;
uniform int   mode;
uniform vec3  viewPos;
uniform Light light;
uniform float specularStrength;
uniform float shininess;
uniform float uvScale;

// Phong reflection model (ambient + diffuse + specular). Two-sided: the normal is flipped
// towards the viewer so flat 2D shapes and open meshes are lit from both faces.
vec3 phong(vec3 N, vec3 P, vec3 albedo)
{
    vec3 V = normalize(viewPos - P);
    if (dot(N, V) < 0.0) N = -N;
    vec3 L = normalize(light.position - P);

    vec3  ambient  = light.ambient * albedo;
    float diff     = max(dot(N, L), 0.0);
    vec3  diffuse  = light.diffuse * diff * albedo;
    vec3  R        = reflect(-L, N);
    float spec     = diff > 0.0 ? pow(max(dot(V, R), 0.0), shininess) : 0.0;
    vec3  specular = light.specular * spec * specularStrength;
    return ambient + diffuse + specular;
}

void main()
{
    vec4 world  = model * vec4(aPos, 1.0);
    FragPos     = world.xyz;
    Normal      = normalMatrix * aNormal;
    VertexColor = aColor;
    UV          = aUV * uvScale;

    if (mode == 1)   // Gouraud: lighting evaluated per vertex, from the vertex color
        GouraudColor = phong(normalize(Normal), FragPos, aColor);
    else
        GouraudColor = vec3(0.0);

    gl_Position = projection * view * world;
}
