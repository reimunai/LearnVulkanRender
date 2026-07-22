#version 450

layout(location = 0) in vec3 inViewPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

//TODO:pbrtexture
layout(binding = 1) uniform sampler2D texSampler;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPos;
// layout (location = 2) out vec4 outColor;

void main()
{
    outAlbedo = texture(texSampler, inTexCoord * 2.0);
    outNormal = vec4(inNormal, 1.0);
    outPos = vec4(inViewPos, 1.0);
    // outColor = vec4(normalize(inTexCoord), 0.0, 1.0);
    // outColor = texture(texSampler, inTexCoord);
}

