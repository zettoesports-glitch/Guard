#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

PixelInput main(ClothVertexInput input)
{
	PixelInput output;
	
	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	
	output.Color = saturate(input.BodyLight);
	float dotr = dot(input.Normal, ChromeWave);
	float2 uv = input.Normal.yz * 0.5f + float2(ChromeWave.y * 3.0f, Wave.x * 3.0f);
	uv.y = -uv.y;
	output.UV = (float2(dotr, 1.0f - dotr) + uv) + input.Data2.xy;
	output.MinAlpha = input.Data.y;

	return output;
}