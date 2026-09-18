#include "GlobalConstantBuffers.inc"
#include "TerrainConstantBuffers.inc"
#include "TerrainHelpers.inc"

struct VertexInput
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD0;
	float2 T : TEXCOORD1;
	float4 Coords : TEXCOORD2;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
	nointerpolation float T : TEXCOORD1;
#if SHADOW_TYPE >= 3
	float4 LightPos : TEXCOORD2;
#endif
	float4 Color : COLOR;
};

PixelInput main( VertexInput input )
{
	PixelInput output;

	output.Position = float4(input.Position, 1.0f);
		
	if(input.T.x >= 0.0f)
	{
		output.Position.y += sin(WindSpeed + input.T.x * 5.0f) * WindScale;
	}
	
	output.Position.z += RequestHeight(uint(input.Coords.x), uint(input.Coords.y));

	output.UV = input.UV.xy;
	output.T = input.T.y;
	output.Color = RequestGrassLight(input.Coords.z, input.Coords.w);

#if SHADOW_TYPE >= 3
	output.LightPos = mul(Shadow, output.Position);
#endif
	output.Position = mul(Game3D, output.Position);

	return output;
}