#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 inViewPos;

layout(set = 1 ,binding = 0) uniform samplerCube cubeSampler;

layout(location = 0) out vec4 outColor;
void main() {
    // outColor = vec4(fragTexCoord, 0.0, 1.0); // Set the output color to red
    vec3 I = normalize(inViewPos);
    vec3 R = reflect(I, normalize(inNormal));
    outColor = vec4(texture(cubeSampler, R).rgb, 1.0);
}
