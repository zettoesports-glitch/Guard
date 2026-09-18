#include "GlobalConstantBuffers.inc"
#include "TerrainConstantBuffers.inc"
#include "TerrainHelpers.inc"

struct VertexInput
{
	uint4 Position : POSITION;
	float2 UV : TEXCOORD0;
	uint VertexIndex : SV_VertexID;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float2 UV1 : TEXCOORD0;
	float2 UV2 : TEXCOORD1;
	nointerpolation float2 W : TEXCOORD2;
	nointerpolation float Water : TEXCOORD3;
#if SHADOW_TYPE >= 3
	float4 LightPos : TEXCOORD4;
#endif
	float4 Color : COLOR;
};

PixelInput main( VertexInput input )
{
	PixelInput output;

	float2 mapping = float2(input.Position.z, input.Position.w);
	uint2 terrainPosition = GetTerrainVertexPosition(input.Position.xy, input.VertexIndex);

	if(IsVisible(terrainPosition.x, terrainPosition.y) == true)
	{
		output.Color = RequestLight(terrainPosition.x, terrainPosition.y);	
		output.W = mapping;
	}
	else
	{	
		output.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
		output.W = float2(-1.0f, -1.0f);
	}

	float4 worldPos = float4(terrainPosition * TERRAIN_SCALE, RequestHeight(terrainPosition.x, terrainPosition.y), 1.0f);
	
#if SHADOW_TYPE >= 3
	output.LightPos = mul(Shadow, worldPos);
#endif
	output.Position = mul(Game3D, worldPos);
	output.Water = 0.0f;
	
	if(mapping.x == WaterIndex1)
	{
		float GrassWind = sin(WindSpeed + terrainPosition.x * 5.0f) * WindScale * 0.002f;

		output.UV1 = float2((terrainPosition.x * input.UV.x) + WaterMove, (terrainPosition.y * input.UV.x) + GrassWind);

		if(mapping.y == WaterIndex1)
		{
			output.UV2 = float2((terrainPosition.x * input.UV.y) + WaterMove, (terrainPosition.y * input.UV.y) + GrassWind);
		}
		else
		{
			output.UV2 = float2(terrainPosition.x * input.UV.y, terrainPosition.y * input.UV.y);
		}
	}
	else
	{
		output.UV1 = float2(terrainPosition.x * input.UV.x, terrainPosition.y * input.UV.x);
		output.UV2 = float2(terrainPosition.x * input.UV.y, terrainPosition.y * input.UV.y);
	}

	return output;
}
