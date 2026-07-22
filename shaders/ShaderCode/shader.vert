#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 outWorldPos;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outWorldPos = (ubo.view * ubo.model * vec4(inPosition, 1.0)).xyz;
    outNormal = mat3(transpose(inverse(ubo.view * ubo.model))) * normal;
    fragTexCoord = inTexCoord;
}
