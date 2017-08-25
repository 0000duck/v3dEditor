#version 450 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(push_constant) uniform Constant
{
    vec2 scale;
    vec2 translate;
}constant;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;

void main()
{
    gl_Position = vec4(inPos * constant.scale + constant.translate, 0.0, 1.0);

    outColor = inColor;
    outUV = inUV;
}
