#include "GlobalConstantBuffers.inc"

struct VertexInput
{
	float3 Position : POSITION;
	float Color : COLOR;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

PixelInput main(VertexInput input)
{
	PixelInput output;

	float color = input.Color;

	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	output.Color = float4(color, 1.0f - color, color, 0.2f);

	return output;
}