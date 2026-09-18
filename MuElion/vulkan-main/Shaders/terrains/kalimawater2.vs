#include "GlobalConstantBuffers.inc"

struct VertexInput
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
#if SHADOW_TYPE >= 3
	float4 LightPos : TEXCOORD1;
#endif
};

static float3 TransformVector = {1.0f, -1.0f, 1.0f};

PixelInput main( VertexInput input )
{
	PixelInput output;	

#if SHADOW_TYPE >= 3
	output.LightPos = mul(Shadow, input.Position);
#endif
	output.Position = mul(Game3D, input.Position);
	
	float lum = 1.0f - dot(input.Normal, TransformVector);
	output.Color = float4(lum, lum * 2.5f, lum * 3.0f, 1.0f);
	output.UV = input.Normal.zy * 0.5f + float2(0.1f, 0.5f);

	return output;
}