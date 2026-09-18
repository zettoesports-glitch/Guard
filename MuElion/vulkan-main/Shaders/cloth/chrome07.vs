#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

PixelInput main(ClothVertexInput input)
{
	PixelInput output;
	
	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	
	output.Color = saturate(input.BodyLight);
	output.UV = (input.Normal.zz + input.Normal.xx) * 0.8f + float2(WorldTime, WorldTime) * 0.00006f;
	output.MinAlpha = input.Data.y;

	return output;
}