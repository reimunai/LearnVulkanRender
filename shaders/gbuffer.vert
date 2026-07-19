#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outViewPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

void main()
{
    outViewPos = (ubo.view * ubo.model * vec4(inPosition, 1.0)).xyz;
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model * ubo.view)));
    outNormal = normalize(normalMatrix * normal);
    outTexCoord = inTexCoord;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}