#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Constant
{
	vec4 eyePos;
	vec4 lightDir;
	vec4 lightColor;
	vec4 cameraParam; // x=nearClip y=farClip z=fadeStart w=(1.0 / fadeMargin)
	vec4 shadowParam; // xy=texelSize z=depth w=dencity
	mat4 invViewProjMatrix;
	mat4 lightMatrix;
}constant;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;   // color
layout(set = 0, binding = 1) uniform sampler2D bufferSampler0; // normal
layout(set = 0, binding = 2) uniform sampler2D bufferSampler1; // material depth
layout(set = 0, binding = 3) uniform sampler2D ssaoSampler;    // ssao
layout(set = 0, binding = 4) uniform sampler2D shadowSampler;  // shadow

layout(location = 0) in vec2 inUV;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outLightColor;
layout(location = 1) out vec4 outGlareColor;

//-------------------------------------------------------------------------------------------------
// function
//-------------------------------------------------------------------------------------------------

vec3 DecomposeWorldPosition(mat4 invViewProjMatrix, vec2 uv, float depth)
{
	vec4 projPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 worldPos = invViewProjMatrix * projPos;

	return worldPos.xyz / worldPos.w;
}

vec3 DecomposeWorldNormal(vec3 normal)
{
	return (normal - 0.5) * 2.0;
}

float ReconstructViewDepth(float depth, vec2 uv, mat4 invProjMatrix)
{
	vec4 projPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
	
	projPos = invProjMatrix * projPos;
	
	return projPos.z / projPos.w;

//	float z = dot(invProjMatrix[2], projPos);
//	float w = dot(invProjMatrix[3], projPos);
	
//	return z / w;
}

float ToLinearDepth(float depth, float nearClip, float farClip)
{
	return (2 * nearClip) / (farClip + nearClip - depth * (farClip - nearClip));
}

//-------------------------------------------------------------------------------------------------
// constant
//-------------------------------------------------------------------------------------------------

#if 0

const vec2 pcOfffsetTable[4] =
{
    {  0.0, -0.5 },
    {  0.0, +0.5 },
    { -0.5,  0.0 },
    { +0.5,  0.0 },
};

const int pcfOffsetCount = 4;
const float pcfMul = 1.0 / float(pcfOffsetCount);

#else

const vec2 pcOfffsetTable[8] =
{
    { -0.5, -0.5 },
    {  0.0, -0.5 },
    { +0.5, -0.5 },
    { -0.5,  0.0 },
    { +0.5,  0.0 },
    { -0.5, +0.5 },
    {  0.0, +0.5 },
    { +0.5, +0.5 },
};

const int pcfOffsetCount = 8;
const float pcfMul = 1.0 / float(pcfOffsetCount);

#endif

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	if(texture(bufferSampler0, inUV).w < 0.5)
	{
		discard;
	}

	vec3 worldNormal = DecomposeWorldNormal(texture(bufferSampler0, inUV).xyz);

	vec4 data = texture(bufferSampler1, inUV);
	float emissiveFactor = data.x;
	float specularFactor = data.y;
	float shininess = data.z;
	float depth = data.w;

	vec3 worldPos = DecomposeWorldPosition(constant.invViewProjMatrix, inUV, depth);

	/*********/
	/* light */
	/*********/

	float NdotL = dot(worldNormal, constant.lightDir.xyz);
	NdotL = NdotL * 0.5 + 0.5;
	NdotL = NdotL * NdotL;
	NdotL = clamp(NdotL, 0.0, 1.0) * constant.lightColor.a;

//	NdotL = mix(NdotL, 1.0, ambient);

	vec3 emissive = texture(colorSampler, inUV).xyz * emissiveFactor; // emissive

	vec3 eyeDir = normalize(constant.eyePos.xyz - worldPos);
	vec3 H = normalize(constant.lightDir.xyz + eyeDir); // specular
	float NdotH = clamp(dot(worldNormal, H), 0.0, 1.0);
	vec3 specular = vec3(specularFactor) * pow(NdotH, shininess);

	vec3 lightColor = constant.lightColor.rgb * NdotL;
	
	/**********/
	/* Shadow */
	/**********/
	
	vec4 shadowPos = constant.lightMatrix * vec4(worldPos, 1.0);
	vec2 shadowCoord = (shadowPos.xy / shadowPos.w) * 0.5 + vec2(0.5, 0.5);
	
	float shadowDepth = shadowPos.z / shadowPos.w;

	float shadowDensity = 0.0;
	vec3 shadowColor = vec3(0.0);

	for(int i = 0; i < pcfOffsetCount; i++)
	{
		vec4 pcfShadow = texture(shadowSampler, shadowCoord + (pcOfffsetTable[i] * constant.shadowParam.xy));

		if(pcfShadow.w + constant.shadowParam.z < shadowDepth)
		{
			shadowDensity += 1.0;
		}

		shadowColor += pcfShadow.rgb;
	}

	shadowDensity = clamp(shadowDensity * pcfMul, 0.0, 1.0);

	shadowColor = clamp(shadowColor * pcfMul, vec3(0.0), vec3(1.0)) * (constant.lightColor.rgb * constant.lightColor.a);
	shadowColor = mix(vec3(1.0), shadowColor, shadowDensity * constant.shadowParam.w);
	
	float linearDepth = ToLinearDepth(depth, constant.cameraParam.x, constant.cameraParam.y);
	if(constant.cameraParam.z <= linearDepth)
	{
		float density = (linearDepth - constant.cameraParam.z) * constant.cameraParam.w;
		shadowColor = mix(shadowColor, vec3(1.0), density);
	}
	
	shadowColor.rgb = min(texture(ssaoSampler, inUV).rgb, shadowColor);

	/**********/
	/* Output */
	/**********/

	// light
	outLightColor.rgb = min(lightColor, shadowColor);
	outLightColor.a = 1.0;
	
	// glare
	outGlareColor.rgb = (emissive + specular) * outLightColor.rgb;
	outGlareColor.a = 1.0f;
}
