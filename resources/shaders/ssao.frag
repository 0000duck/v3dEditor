#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Constant
{
	vec4 params0;
	vec4 params1;
}constant;

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D bufferSampler0;
layout(set = 0, binding = 1) uniform sampler2D bufferSampler1;
layout(set = 0, binding = 2) uniform sampler2D noiseSampler;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// function
//-------------------------------------------------------------------------------------------------

float LinearizeDepth(float depth, float near, float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

const int samplingCount = 16;

const float invSamplingCount = 1.0 / 16.0;//float(samplingCount);

const vec3 samplingTable[16] =
{
	vec3( +0.53812504,  +0.18565957,   -0.43192      ),
	vec3( +0.13790712,  +0.24864247,   +0.44301823   ),
	vec3( +0.33715037,  +0.56794053,   -0.005789503  ),
	vec3( -0.6999805,   -0.04511441,   -0.0019965635 ),
	vec3( +0.06896307,  -0.15983082,   -0.85477847   ),
	vec3( +0.056099437, +0.006954967,  -0.1843352    ),
	vec3( -0.014653638, +0.14027752,   +0.0762037    ),
	vec3( +0.010019933, -0.1924225,    -0.034443386  ),
	vec3( -0.35775623,  -0.5301969,    -0.43581226   ),
	vec3( -0.3169221,   +0.106360726,  +0.015860917  ),
	vec3( +0.010350345, -0.58698344,   +0.0046293875 ),
	vec3( -0.08972908,  -0.49408212,   +0.3287904    ),
	vec3( +0.7119986,   -0.0154690035, -0.09183723   ),
	vec3( -0.053382345, +0.059675813,  -0.5411899    ),
	vec3( +0.035267662, -0.063188605,  +0.54602677   ),
	vec3( -0.47761092,  +0.2847911,    -0.0271716    ),
};

#define RADIUS      constant.params0.x
#define THRESHOLD   constant.params0.y
#define DEPTH       constant.params0.z
#define INTENSITY   constant.params0.w
#define NTEX_ASPECT constant.params1.xy
#define NEAR_CLIP   constant.params1.z
#define FAR_CLIP    constant.params1.w

void main()
{
	vec4 normal = texture(bufferSampler0, inUV);
	if(normal.w < 0.5)
	{
		discard;
	}

	normal.xyz = normal.xyz * 2.0 - 1.0;
	
	float depth = LinearizeDepth(texture(bufferSampler1, inUV).w, NEAR_CLIP, FAR_CLIP);
	vec3 pos = vec3(inUV, depth);
	vec3 noise = normalize(texture(noiseSampler, inUV * NTEX_ASPECT).xyz * 2.0 - 1.0);
	float rd = RADIUS / depth;

	float ao = 0.0;
	
	for(int i = 0; i < samplingCount; i++)
	{
		vec3 sampleRay = reflect(samplingTable[i].xyz, noise) * rd;

		vec3 samplePos = pos + sampleRay * sign(dot(sampleRay, normal.xyz));

		float sampleDepth = LinearizeDepth(texture(bufferSampler1, samplePos.xy).w, NEAR_CLIP, FAR_CLIP);
		vec3 sampleNormal = texture(bufferSampler0, samplePos.xy).xyz * 2.0 - 1.0;
		
		float diffDepth = depth - sampleDepth;
		float diffNormal = 1.0 - dot(normal.xyz, sampleNormal);
		
//		ao += step(THRESHOLD, diffDepth) * (1.0 - smoothstep(THRESHOLD, DEPTH, diffDepth));
		ao += step(THRESHOLD, diffDepth) * diffNormal * (1.0 - smoothstep(THRESHOLD, DEPTH, diffDepth));
	}
	
	ao = 1.0 - clamp(ao * invSamplingCount * INTENSITY, 0.0, 1.0);
	
	outColor = vec4(vec3(ao), 1.0);
}
