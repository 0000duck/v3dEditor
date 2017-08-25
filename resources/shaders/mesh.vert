#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Scene
{
	mat4 viewProjMatrix;
}scene;

layout(std140, set = 0, binding = 0) uniform Mesh
{
#ifdef SKELETAL_ENABLE
	mat4 worldMatrices[BONE_COUNT];
#else //SKELETAL_ENABLE
	mat4 worldMatrix;
#endif //SKELETAL_ENABLE
	uint key;
}mesh;

layout(location = 0) in vec3 inPos;
#ifdef TEXTURE_ENABLE
layout(location = 1) in vec2 inUV;
#endif //TEXTURE_ENABLE
layout(location = 2) in vec3 inNormal;
#ifdef BUMP_TEXTURE_ENABLE
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;
#endif //BUMP_TEXTURE_ENABLE
#ifdef SKELETAL_ENABLE
layout(location = 5) in ivec4 inIndices;
layout(location = 6) in vec4 inWeights;
#endif //SKELETAL_ENABLE

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

#ifdef TEXTURE_ENABLE
layout(location = 0) out vec2 outUV;
#endif //TEXTURE_ENABLE
#ifdef TRANSPARENCY_ENABLE
layout(location = 1) out vec4 outWorldPos;
#else //TRANSPARENCY_ENABLE
layout(location = 1) out vec4 outProjPos;
#endif //TRANSPARENCY_ENABLE
layout(location = 2) out vec3 outWorldNormal;
#ifdef BUMP_TEXTURE_ENABLE
layout(location = 3) out vec3 outWorldTangent;
layout(location = 4) out vec3 outWorldBinormal;
#endif //BUMP_TEXTURE_ENABLE

out gl_PerVertex 
{
   vec4 gl_Position;
};

//-------------------------------------------------------------------------------------------------
// functions
//-------------------------------------------------------------------------------------------------

#ifdef SKELETAL_ENABLE

// World transform position
vec4 BoneTransformPosition(vec4 pos)
{
	vec4 result =	((mesh.worldMatrices[inIndices.x] * pos) * inWeights.x) +
					((mesh.worldMatrices[inIndices.y] * pos) * inWeights.y) +
					((mesh.worldMatrices[inIndices.z] * pos) * inWeights.z) +
					((mesh.worldMatrices[inIndices.w] * pos) * inWeights.w);

	return vec4(result.xyz, 1.0);
}

// World transform normal
vec3 BoneTransformNormal(vec3 normal)
{
	vec3 result =	((mat3(mesh.worldMatrices[inIndices.x]) * normal) * inWeights.x) +
					((mat3(mesh.worldMatrices[inIndices.y]) * normal) * inWeights.y) +
					((mat3(mesh.worldMatrices[inIndices.z]) * normal) * inWeights.z) +
					((mat3(mesh.worldMatrices[inIndices.w]) * normal) * inWeights.w);

	return result;
}
	
#endif //SKELETAL_ENABLE

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	/******/
	/* UV */
	/******/

#ifdef TEXTURE_ENABLE
	outUV = inUV;
#endif //TEXTURE_ENABLE

	/************/
	/* Position */
	/************/

#ifdef SKELETAL_ENABLE
	vec4 worldPos = BoneTransformPosition(vec4(inPos, 1.0));
	gl_Position = scene.viewProjMatrix * worldPos;
#else //SKELETAL_ENABLE
	vec4 worldPos = mesh.worldMatrix * vec4(inPos, 1.0);
	gl_Position = scene.viewProjMatrix * worldPos;
#endif //SKELETAL_ENABLE

#ifdef TRANSPARENCY_ENABLE
	outWorldPos = worldPos;
#else //TRANSPARENCY_ENABLE
	outProjPos = gl_Position;
#endif //TRANSPARENCY_ENABLE
	
	/****************/
	/* World normal */
	/****************/

#ifdef SKELETAL_ENABLE
	outWorldNormal = BoneTransformNormal(inNormal);
#else //SKELETAL_ENABLE
	outWorldNormal = mat3(mesh.worldMatrix) * inNormal;
#endif //SKELETAL_ENABLE

	/*****************/
	/* Tangent space */
	/*****************/

#ifdef BUMP_TEXTURE_ENABLE
#ifdef SKELETAL_ENABLE
	outWorldTangent = BoneTransformNormal(inTangent);
	outWorldBinormal = BoneTransformNormal(inBinormal);
#else //SKELETAL_ENABLE
	outWorldTangent = mat3(mesh.worldMatrix) * inTangent;
	outWorldBinormal = mat3(mesh.worldMatrix) * inBinormal;
#endif //SKELETAL_ENABLE
#endif //BUMP_TEXTURE_ENABLE
}
