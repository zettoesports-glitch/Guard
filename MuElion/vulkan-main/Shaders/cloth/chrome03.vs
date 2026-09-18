#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

static const float3 TransformVector = { 0.0f, -0.1f, -0.8f };

PixelInput main(ClothVertexInput input)
{
	PixelInput output;
	
	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	
	output.Color = saturate(input.BodyLight);
	float dotr = dot(input.Normal, TransformVector);
	output.UV = float2(dotr, 1.0f - dotr);
	output.MinAlpha = input.Data.y;

	return output;
}