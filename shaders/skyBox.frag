#version 450

layout(location = 0) in vec3 inPos;

layout(set = 1 ,binding = 0) uniform samplerCube cubeSampler;

layout(location = 0) out vec4 outColor;
void main()
{
    // outColor = texture(cubeSampler, inPos);
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}