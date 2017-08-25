#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Constant
{
	vec4 texelSize;
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

const vec2 offsetTable[24] =
{
	{ -2.0, -2.0 },
	{ -1.0, -2.0 },
	{  0.0, -2.0 },
	{ +1.0, -2.0 },
	{ +2.0, -2.0 },

	{ -2.0, -1.0 },
	{ -1.0, -1.0 },
	{  0.0, -1.0 },
	{ +1.0, -1.0 },
	{ +2.0, -1.0 },

	{ -2.0, 0.0 },
	{ -1.0, 0.0 },
	{ +1.0, 0.0 },
	{ +2.0, 0.0 },

	{ -2.0, +1.0 },
	{ -1.0, +1.0 },
	{  0.0, +1.0 },
	{ +1.0, +1.0 },
	{ +2.0, +1.0 },

	{ -2.0, +2.0 },
	{ -1.0, +2.0 },
	{  0.0, +2.0 },
	{ +1.0, +2.0 },
	{ +2.0, +2.0 },
};

const float averageValue = 1.0 / 24.0;

void main()
{
	if(texture(textureSampler, inUV).a > 0.5)
	{
		discard;
	}
	
	vec4 color = vec4(0.0);
	
	for(int i = 0; i < 24; i++)
	{
		color += texture(textureSampler, inUV + constant.texelSize.xy * offsetTable[i]);
	}
	
	outColor = (color.a > 0.0004)? constant.color : vec4(0.0);
}
