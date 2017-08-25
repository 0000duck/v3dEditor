#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Param
{
	vec2 texelSize;
}param;

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

const vec2 offsetTable[4] =
{
    {  0.5, -0.5 },
    {  0.5,  0.5 },
    { -0.5, -0.5 },
    { -0.5,  0.5 },
};

const float averageValue = 1.0 / 4.0;

void main()
{
	vec4 color = vec4(0.0);
	
	for(int i = 0; i < 4; i++)
	{
		color += texture(textureSampler, inUV + param.texelSize * offsetTable[i]);
	}
	
	outColor = color * averageValue;
}
