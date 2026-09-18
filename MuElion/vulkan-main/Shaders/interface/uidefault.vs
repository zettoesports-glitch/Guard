#include "GlobalConstantBuffers.inc"

struct VertexInputType
{
	float2 Position : POSITION;
	float2 UV : TEXCOORD0;
	float4 Color : COLOR0;
};

struct PixelOutputType
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
	float4 Color : COLOR0;
};

PixelOutputType main( VertexInputType input )
{
	PixelOutputType output;

	output.Position = mul(Game2D, float4(input.Position, 0.0f, 1.0f));
	output.Position.y = -output.Position.y;
	output.UV = input.UV;
	output.Color = input.Color;

	return output;
}
