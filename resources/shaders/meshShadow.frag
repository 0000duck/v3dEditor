#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(std140, set = 1, binding = 0) uniform Material
{
	vec4 diffuseColor;
	vec4 specularColor;
	float diffuseFactor;
	float specularFactor;
	float shininess;
	float emissiveFactor;
}material;

#ifdef DIFFUSE_TEXTURE_ENABLE
layout(set = 1, binding = 1) uniform sampler2D diffuseSampler;
#endif //DIFFUSE_TEXTURE_ENABLE

layout(location = 0) in vec4 inProjPos;
#ifdef TEXTURE_ENABLE
layout(location = 1) in vec2 inUV;
#endif //TEXTURE_ENABLE

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outData;

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
#ifdef DIFFUSE_TEXTURE_ENABLE
	vec4 color = material.diffuseColor * texture(diffuseSampler, inUV);
#else //DIFFUSE_TEXTURE_ENABLE
	vec4 color = material.diffuseColor;
#endif //DIFFUSE_TEXTURE_ENABLE
	if(color.a < 0.0001)
	{
		discard;
	}
	
	color.rgb = mix(color.rgb * (1.0 - color.a), vec3(0.25), color.a);

	float depth = inProjPos.z / inProjPos.w;

	outData = vec4(color.rgb, depth);
}
