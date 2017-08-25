#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Param
{
	vec4 step;
	vec4 inc;
	int sampleCount;
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

void main()
{
	vec2 offset = param.step.xy;
	vec3 inc = param.inc.xyz;

	outColor = texture(textureSampler, inUV) * inc.x;

	for( int i = 1; i < param.sampleCount; i++ )
	{
		inc.xy *= inc.yz;

		outColor += texture(textureSampler, inUV - offset) * inc.x;
		outColor += texture(textureSampler, inUV + offset) * inc.x;

		offset += param.step.xy;
	}
}
