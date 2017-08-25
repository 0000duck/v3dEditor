#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(location = 0) in vec2 inUV;

layout(push_constant) uniform Constant
{
	vec4 texOffset0; // xy=( -1.0 / W, -1.0 / H ) zw=( +1.0 / W, -1.0 / H )
	vec4 texOffset1; // xy=( -1.0 / W, +1.0 / H ) zw=( +1.0 / W, +1.0 / H )
	vec4 invTexSize;
}constant;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

const vec3 LUM_VECTOR = { 0.299, 0.587, 0.114 };

const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_REDUCE_MIN = 1.0 / 128.0;

void main()
{
	vec4 color = texture(colorSampler, inUV);

	vec3 rgbNW = texture(colorSampler, inUV + constant.texOffset0.xy).rgb;
	vec3 rgbNE = texture(colorSampler, inUV + constant.texOffset0.zw).rgb;
	vec3 rgbSW = texture(colorSampler, inUV + constant.texOffset1.xy).rgb;
	vec3 rgbSE = texture(colorSampler, inUV + constant.texOffset1.zw).rgb;
	vec3 rgbM  = color.rgb;

	float lumaNW = dot(rgbNW, LUM_VECTOR);
	float lumaNE = dot(rgbNE, LUM_VECTOR);
	float lumaSW = dot(rgbSW, LUM_VECTOR);
	float lumaSE = dot(rgbSE, LUM_VECTOR);
	float lumaM  = dot(rgbM,  LUM_VECTOR);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	vec2 dir;
	float dirReduce;
	float rcpDirMin;
	vec3 rgbA;
	vec3 rgbB;
	float lumaB;

	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

	rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

	dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX ), max( vec2( -FXAA_SPAN_MAX, -FXAA_SPAN_MAX ), dir * rcpDirMin ) ) * constant.invTexSize.xy;

	rgbA = ( 1.0 / 2.0 ) * ( texture(colorSampler, inUV + dir * ( 1.0 / 3.0 - 0.5 ) ).rgb +
	                         texture(colorSampler, inUV + dir * ( 2.0 / 3.0 - 0.5 ) ).rgb );

	rgbB = rgbA * ( 1.0 / 2.0 ) + ( 1.0 / 4.0 ) * ( texture(colorSampler, inUV + dir * ( 0.0 / 3.0 - 0.5 ) ).rgb +
	                                                texture(colorSampler, inUV + dir * ( 3.0 / 3.0 - 0.5 ) ).rgb );

	lumaB = dot( rgbB, LUM_VECTOR );

	if( ( lumaB < lumaMin ) ||
	    ( lumaB > lumaMax ) )
	{
		outColor = vec4( rgbA, color.a );
	}
	else
	{
		outColor = vec4( rgbB, color.a );
	}
}
