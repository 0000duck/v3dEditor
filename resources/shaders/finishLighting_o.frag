#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;
layout(set = 0, binding = 1) uniform sampler2D lightSampler;
layout(set = 0, binding = 2) uniform sampler2D glareSampler;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	vec4 lightColor = texture(lightSampler, inUV);

	if(lightColor.w < 0.5)
	{
		outColor = texture(colorSampler, inUV);
	}
	else
	{
		outColor = texture(colorSampler, inUV) * lightColor + vec4(texture(glareSampler, inUV).rgb, 0.0);
	}
}
