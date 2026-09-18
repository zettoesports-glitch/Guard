#include "GlobalConstantBuffers.inc"

struct VertexInput
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
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
	output.UV = input.Normal.zy * 0.5f + float2(0.1f, 0.5f);

	return output;
}