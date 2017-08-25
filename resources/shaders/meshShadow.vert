#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Scene
{
	mat4 lightMatrix;
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
#ifdef SKELETAL_ENABLE
layout(location = 5) in ivec4 inIndices;
layout(location = 6) in vec4 inWeights;
#endif //SKELETAL_ENABLE

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outProjPos;
#ifdef TEXTURE_ENABLE
layout(location = 1) out vec2 outUV;
#endif //TEXTURE_ENABLE

out gl_PerVertex 
{
   vec4 gl_Position;
};

//-------------------------------------------------------------------------------------------------
// function
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

#endif //SKELETAL_ENABLE

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
#ifdef SKELETAL_ENABLE
	vec4 worldPos = BoneTransformPosition(vec4(inPos, 1.0));
#else //SKELETAL_ENABLE
	vec4 worldPos = mesh.worldMatrix * vec4(inPos, 1.0);
#endif //SKELETAL_ENABLE
	
	gl_Position = scene.lightMatrix * worldPos;

	outProjPos = gl_Position;
#ifdef TEXTURE_ENABLE
	outUV = inUV;
#endif //TEXTURE_ENABLE
}
