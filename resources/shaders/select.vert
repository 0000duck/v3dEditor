#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Scene
{
	layout(offset = 0) mat4 viewProjMatrix;
}scene;

layout(std140, set = 0, binding = 0) uniform Mesh
{
	mat4 worldMatrix;
}mesh;

layout(location = 0) in vec3 inPos;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out uint outKey;

out gl_PerVertex 
{
   vec4 gl_Position;
};

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	mat4 worldViewProjMatrix = scene.viewProjMatrix * mesh.worldMatrix;
	
	gl_Position = worldViewProjMatrix * vec4(inPos, 1.0);
}
