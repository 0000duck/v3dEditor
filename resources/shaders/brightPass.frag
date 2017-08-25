#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//-------------------------------------------------------------------------------------------------
// input
//-------------------------------------------------------------------------------------------------

layout(push_constant) uniform Constant
{
	float threshold;
	float offset;
}constant;

layout(set = 0, binding = 0) uniform sampler2D colorSampler;

layout(location = 0) in vec2 inUV;

//-------------------------------------------------------------------------------------------------
// output
//-------------------------------------------------------------------------------------------------

layout(location = 0) out vec4 outColor;

//-------------------------------------------------------------------------------------------------
// function
//-------------------------------------------------------------------------------------------------

const float epsilon = 0.0000001;

bool IsZero(float value)
{
	if((-epsilon <= value) && (value <= +epsilon))
	{
		return true;
	}
	
	return false;
}

vec3 RGBToYxy(vec3 rgb)
{
	const mat3 MAT = mat3(0.4124, 0.2126, 0.0193, 0.3576, 0.7152, 0.1192, 0.1805, 0.0722, 0.9505);
    
	vec3 xyz = MAT * rgb;
	vec3 Yxy;
	Yxy.r = xyz.g;

	float temp = dot(vec3(1.0), xyz);
	Yxy.gb = (IsZero(temp) == false)? (xyz.rg / temp) : vec2(0.0);

	return Yxy;
}

vec3 YxyToRGB(vec3 Yxy)
{
	const mat3 MAT = mat3(3.2405, -0.9693, 0.0556, -1.5371, 1.8760, -0.2040, -0.4985, 0.0416, 1.0572);

	vec3 xyz;

	xyz.r = (IsZero(Yxy.b) == false)? (Yxy.r * Yxy.g / Yxy.b) : 0.0;
	xyz.g = Yxy.r;
	xyz.b = (IsZero(Yxy.b) == false)? (Yxy.r * (1.0 - Yxy.g - Yxy.b) / Yxy.b) : 0.0;

	return MAT * xyz;
}

//-------------------------------------------------------------------------------------------------
// entry
//-------------------------------------------------------------------------------------------------

void main()
{
	vec4 color = texture(colorSampler, inUV);
	vec3 Yxy = RGBToYxy(color.rgb);
	
	Yxy.r = max(0.0, Yxy.r - constant.threshold);
	
	float Y = constant.offset + Yxy.r;
	Yxy.r = (IsZero(Y) == false)? (Yxy.r / (constant.offset + Yxy.r)) : 0.0;

	outColor.rgb = YxyToRGB(Yxy);
	outColor.a = color.a;
}
