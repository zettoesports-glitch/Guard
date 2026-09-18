#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

PixelInput main(ClothVertexInput input)
{
	PixelInput output;

	output.Position = mul(Game3D, float4(input.Position, 1.0f));
	output.UV = input.UV;
	output.MinAlpha = input.Data.y;

	if(input.Data.x == 1.0f)
	{
		float Luminosity = dot(input.Normal, LightPosition) * 0.8f + 0.4f;

		if(Luminosity < 0.2f)
		{
			Luminosity = 0.2f;
		}

		output.Color = saturate(input.BodyLight * float4(Luminosity, Luminosity, Luminosity, 1.0f));
	}
	else
	{
		output.Color = saturate(input.BodyLight);
	}

	return output;
}