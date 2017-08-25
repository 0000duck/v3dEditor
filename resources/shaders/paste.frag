#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Constant
{
	vec4 color;
}constant;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 inUV;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	outColor = texture(textureSampler, inUV) * constant.color;
}