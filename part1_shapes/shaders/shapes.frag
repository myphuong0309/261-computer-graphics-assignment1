#version 330 core
// Part 1 fragment shader. mode:
//   0 flat color (unlit)            3 texture x Phong lighting
//   1 Gouraud (color from the VS)   4 wireframe (unlit object color)
//   2 Phong (per-fragment)          5 unlit vertex color (grid / axes)

in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;
in vec2 UV;
in vec3 GouraudColor;

out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform int       mode;
uniform vec3      viewPos;
uniform Light     light;
uniform vec3      objectColor;
uniform float     specularStrength;
uniform float     shininess;
uniform sampler2D tex;

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
    vec3 color;
    if      (mode == 0) color = objectColor;
    else if (mode == 1) color = GouraudColor;
    else if (mode == 2) color = phong(normalize(Normal), FragPos, objectColor);
    else if (mode == 3) color = phong(normalize(Normal), FragPos, texture(tex, UV).rgb);
    else if (mode == 4) color = objectColor;
    else                color = VertexColor;
    FragColor = vec4(color, 1.0);
}
