#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"

#if SHADOW_TYPE <= 2
#include "TerrainModelHelpers.inc"
#endif

ShadowPixelInput main(ClothVertexInput input)
{
	ShadowPixelInput output;

	if(input.Data2.z == DISABLED_SHADOW)
	{
		output.Position = float4(0.0f, 0.0f, 0.0f, 0.0f);
		output.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
#if SHADOW_TYPE <= 2
		output.Color2 = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
		output.UV = float2(0.0f, 0.0f);
		output.MinAlpha = 1.0f;
	}
	else
	{
#if SHADOW_TYPE >= 3
		output.Position = mul(Shadow, float4(input.Position, 1.0f));
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
#else
		float4 bodyOrigin = float4(input.BodyOrigin.xy, input.Data2.w, 0.0f);
			
		output.Position = float4(input.Position, 1.0f);

		output.Position -= bodyOrigin;
		output.Position.x += output.Position.z * (output.Position.x + ShadowPosition.x) / (output.Position.z - ShadowPosition.y);
		output.Position.z = 5.0f;
		output.Position += bodyOrigin;

		output.Position = mul(Game3D, output.Position);
		output.UV = input.UV;
		output.MinAlpha = input.Data.y;

		output.Color2 = input.ShadowLight;

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
#endif
	}

	return output;
}