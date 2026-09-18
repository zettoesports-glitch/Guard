#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

PixelInput main(ClothVertexInput input)
{
	PixelInput output;
	
	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	
	output.Color = saturate(input.BodyLight);
	float dotr = dot(input.Normal, ChromeWave);
	float2 uv = input.Normal.yz * float2(3.0f, 2.5f) + float2(ChromeWave.y * 5.0f, Wave.x);
	uv.y = -uv.y;
	output.UV = float2(dotr, 1.0f - dotr) + uv;
	output.MinAlpha = input.Data.y;

	return output;
}