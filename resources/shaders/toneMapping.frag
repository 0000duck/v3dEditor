#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// function
//-------------------------------------------------------------------------------------------------

const float FH_EXP_BIAS = 2.0; //ExposureBias
const float FH_A = 0.22; //ShoulderStrength
const float FH_B = 0.30; //LinearStrength
const float FH_C = 0.10; //LinearAngle
const float FH_D = 0.20; //ToeStrength
const float FH_E = 0.01; //ToeNumerator
const float FH_F = 0.30; //ToeDenominator
const float FH_W = 11.2; //LinearWhitePointValue

vec3 tonemap(vec3 x)
{
	return ( ( x * ( FH_A * x + FH_C * FH_B ) + FH_D * FH_E ) / ( x * ( FH_A * x + FH_B ) + FH_D * FH_F ) ) - FH_E / FH_F;
}

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	outColor = texture(colorSampler, inUV);

	vec3 current = tonemap(FH_EXP_BIAS * outColor.rgb);
	vec3 whiteScale = 1.0 / tonemap(vec3(FH_W));

	outColor.rgb = current * whiteScale;
//	outColor.rgb = pow(abs(outColor.rgb), vec3(1.0 / 2.2));
}
