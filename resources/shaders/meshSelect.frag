#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(std140, set = 0, binding = 0) uniform Mesh
{
#ifdef SKELETAL_ENABLE
	mat4 worldMatrices[BONE_COUNT];
#else //SKELETAL_ENABLE
	mat4 worldMatrix;
#endif //SKELETAL_ENABLE
	uint key;
}mesh;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// entry - transparency
//-------------------------------------------------------------------------------------------------

void main()
{
	outColor = vec4(1.0);
}
