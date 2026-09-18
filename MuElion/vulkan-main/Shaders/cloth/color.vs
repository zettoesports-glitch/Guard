#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

ColorPixelInput main(ClothVertexInput input)
{
	ColorPixelInput output;
	
	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	
	output.Color = saturate(input.BodyLight);
	output.MinAlpha = input.Data.y;

	return output;
}