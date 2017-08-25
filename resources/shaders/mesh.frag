#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

#ifdef TRANSPARENCY_ENABLE
layout(push_constant) uniform Scene
{
	layout(offset = 64) vec4 eyePos;
	layout(offset = 80) vec4 lightDir;
	layout(offset = 96) vec4 lightColor;
}scene;
#endif //TRANSPARENCY_ENABLE

layout(std140, set = 0, binding = 0) uniform Mesh
{
#ifdef SKELETAL_ENABLE
	mat4 worldMatrices[BONE_COUNT];
#else //SKELETAL_ENABLE
	mat4 worldMatrix;
#endif //SKELETAL_ENABLE
	uint key;
}mesh;

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

#ifdef SPECULAR_TEXTURE_ENABLE
layout(set = 1, binding = 2) uniform sampler2D specularSampler;
#endif //SPECULAR_TEXTURE_ENABLE

#ifdef BUMP_TEXTURE_ENABLE
layout(set = 1, binding = 3) uniform sampler2D bumpSampler;
#endif //BUMP_TEXTURE_ENABLE

#ifdef TEXTURE_ENABLE
layout(location = 0) in vec2 inUV;
#endif //TEXTURE_ENABLE

#ifdef TRANSPARENCY_ENABLE
layout(location = 1) in vec4 inWorldPos;
#else //TRANSPARENCY_ENABLE
layout(location = 1) in vec4 inProjPos;
#endif //TRANSPARENCY_ENABLE

layout(location = 2) in vec3 inWorldNormal;

#ifdef BUMP_TEXTURE_ENABLE
layout(location = 3) in vec3 inWorldTangent;
layout(location = 4) in vec3 inWorldBinormal;
#endif //BUMP_TEXTURE_ENABLE

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

#ifdef TRANSPARENCY_ENABLE
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outGlare;
layout(location = 2) out vec4 outSelect;
#else //TRANSPARENCY_ENABLE
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBuffer0;
layout(location = 2) out vec4 outBuffer1;
layout(location = 3) out vec4 outSelect;
#endif //TRANSPARENCY_ENABLE

//-------------------------------------------------------------------------------------------------
// function
//-------------------------------------------------------------------------------------------------

vec3 TransformNormal(vec3 localNormal, vec3 worldNormal, vec3 worldTangent, vec3 worldBinormal)
{
	mat3 tarnsformMat = mat3(normalize(worldTangent), normalize(worldBinormal), normalize(worldNormal));
	
	return normalize(tarnsformMat * localNormal);
}

float ToLuminance(vec3 color)
{
	const mat3 MAT = mat3(0.4124, 0.2126, 0.0193, 0.3576, 0.7152, 0.1192, 0.1805, 0.0722, 0.9505);
	vec3 xyz = MAT * color;
	return xyz.g;
}

#ifdef TRANSPARENCY_ENABLE

//-------------------------------------------------------------------------------------------------
// entry - transparency
//-------------------------------------------------------------------------------------------------

void main()
{
#ifdef BUMP_TEXTURE_ENABLE
	vec3 localNormal = texture(bumpSampler, inUV).xyz * 2.0 - 1.0;
	vec3 worldNormal = TransformNormal(normalize(localNormal), normalize(inWorldNormal), normalize(inWorldTangent), normalize(inWorldBinormal));
#else //BUMP_TEXTURE_ENABLE
	vec3 worldNormal = normalize(inWorldNormal);
#endif //BUMP_TEXTURE_ENABLE

	/************/
	/* material */
	/************/

#ifdef DIFFUSE_TEXTURE_ENABLE
	vec4 diffuseColor = material.diffuseColor * texture(diffuseSampler, inUV);
#else //DIFFUSE_TEXTURE_ENABLE
	vec4 diffuseColor = material.diffuseColor;
#endif //DIFFUSE_TEXTURE_ENABLE
	diffuseColor.rgb = diffuseColor.rgb * material.diffuseFactor;
	
	float NdotL = clamp(dot(worldNormal, scene.lightDir.xyz), 0.0, 1.0);
	NdotL = NdotL * 0.5 + 0.5;
	NdotL = NdotL * NdotL;
	NdotL = NdotL * scene.lightColor.a;

	// light
	vec3 lightColor = scene.lightColor.rgb * NdotL;

	// specular
	vec3 eyeDir = normalize(scene.eyePos.xyz - inWorldPos.xyz);
	vec3 H = normalize(scene.lightDir.xyz + eyeDir);
	float NdotH = clamp(dot(worldNormal, H), 0.0, 1.0);
#ifdef SPECULAR_TEXTURE_ENABLE
	vec3 specularColor = vec3(ToLuminance(material.specularColor.rgb + texture(specularSampler, inUV).rgb));
#else //SPECULAR_TEXTURE_ENABLE
	vec3 specularColor = vec3(ToLuminance(material.specularColor.rgb));
#endif //SPECULAR_TEXTURE_ENABLE
	specularColor = specularColor * material.specularFactor * pow(NdotH, material.shininess);

	// emissive
	vec3 emissiveColor = diffuseColor.rgb * material.emissiveFactor;

	// output
	outColor.rgb = diffuseColor.rgb * lightColor + specularColor + emissiveColor;
	outColor.a = diffuseColor.a;
	
	/*********/
	/* Glare */
	/*********/
	
	outGlare.rgb = (specularColor + emissiveColor) * lightColor;
	outGlare.a = 1.0;

	/**********/
	/* Select */
	/**********/

	outSelect = unpackUnorm4x8(mesh.key);
}

#else //TRANSPARENCY_ENABLE

//-------------------------------------------------------------------------------------------------
// entry - opacity
//-------------------------------------------------------------------------------------------------

void main()
{
#ifdef DIFFUSE_TEXTURE_ENABLE
	vec4 texDiffuseColor = texture(diffuseSampler, inUV);
	if(texDiffuseColor.a < 0.00001)
	{
		discard;
	}
#endif //DIFFUSE_TEXTURE_ENABLE
	
#ifdef BUMP_TEXTURE_ENABLE
	vec3 localNormal = texture(bumpSampler, inUV).xyz * 2.0 - 1.0;
	vec3 worldNormal = TransformNormal(normalize(localNormal), normalize(inWorldNormal), normalize(inWorldTangent), normalize(inWorldBinormal));
#endif //BUMP_TEXTURE_ENABLE
	
#ifdef DIFFUSE_TEXTURE_ENABLE
	outColor = material.diffuseColor * texDiffuseColor;
#else //DIFFUSE_TEXTURE_ENABLE
	outColor = material.diffuseColor;
#endif //DIFFUSE_TEXTURE_ENABLE
	outColor.rgb = outColor.rgb * material.diffuseFactor;

#ifdef BUMP_TEXTURE_ENABLE
	outBuffer0 = vec4(worldNormal * 0.5 + 0.5, 1.0);
#else //BUMP_TEXTURE_ENABLE
	outBuffer0 = vec4(inWorldNormal * 0.5 + 0.5, 1.0);
#endif //BUMP_TEXTURE_ENABLE

	/************/
	/* material */
	/************/

	outBuffer1.x = material.emissiveFactor;
	
#ifdef SPECULAR_TEXTURE_ENABLE
	outBuffer1.y = ToLuminance(material.specularColor.rgb + texture(specularSampler, inUV).rgb) * material.specularFactor;
#else //SPECULAR_TEXTURE_ENABLE
	outBuffer1.y = ToLuminance(material.specularColor.rgb) * material.specularFactor;
#endif //SPECULAR_TEXTURE_ENABLE

	outBuffer1.z = material.shininess;

	/*********/
	/* depth */
	/*********/
	
	outBuffer1.w = inProjPos.z / inProjPos.w;
	
	/**********/
	/* Select */
	/**********/
	
	outSelect = unpackUnorm4x8(mesh.key);
}

#endif //TRANSPARENCY_ENABLE