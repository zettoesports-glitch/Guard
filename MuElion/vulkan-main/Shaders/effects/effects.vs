#include "GlobalConstantBuffers.inc"
#include "TerrainHelpers.inc"

struct VertexInput
{
	uint VertexId : SV_VertexID;
	[[vk::location(0)]] float4 Instance0 : INSTANCE_MATRIX0;
	[[vk::location(1)]] float4 Instance1 : INSTANCE_MATRIX1;
	[[vk::location(2)]] float4 Instance2 : INSTANCE_MATRIX2;
	[[vk::location(3)]] float4 Instance3 : INSTANCE_MATRIX3;
	[[vk::location(4)]] float4 UV : INSTANCE0;
	[[vk::location(5)]] float2 Size : INSTANCE1;
	[[vk::location(6)]] float4 Color : INSTANCE2;
	[[vk::location(7)]] uint4 Data : INSTANCE3;
};

struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
	nointerpolation float2 Tex : TEXCOORD1;
};

float3 RequestSelector(uint VertexId)
{
	switch(VertexId % 6)
	{
	case 0: return float3(-0.5f, -0.5f, 0.0f);
	case 1: return float3( 0.5f, -0.5f, 1.0f);
	case 2: return float3( 0.5f,  0.5f, 2.0f);
	case 3: return float3(-0.5f, -0.5f, 0.0f);
	case 4: return float3( 0.5f,  0.5f, 2.0f);
	default: return float3(-0.5f, 0.5f, 3.0f);
	}
}

float4 RequestInstanceColumn(VertexInput input, uint Index)
{
	switch(Index)
	{
	case 0: return input.Instance0;
	case 1: return input.Instance1;
	case 2: return input.Instance2;
	default: return input.Instance3;
	}
}

float4 TransformInstancePosition(VertexInput input, float4 Position)
{
	return input.Instance0 * Position.x +
		input.Instance1 * Position.y +
		input.Instance2 * Position.z +
		input.Instance3 * Position.w;
}

PixelInput main(VertexInput input)
{
	PixelInput output;
	float3 selector = RequestSelector(input.VertexId);
	uint corner = (uint)selector.z;
	float4 instanceVertex = RequestInstanceColumn(input, corner);
	
	if(input.Data.z == 4 || input.Data.z == 5)
	{
		float4 instanceOrigin = RequestInstanceColumn(input, 0);
		float2 terrainOffset = float2(selector.x > 0.0f ? TERRAIN_SCALE : 0.0f,
			selector.y > 0.0f ? TERRAIN_SCALE : 0.0f);
		output.Position = float4(instanceOrigin.xy + terrainOffset,
			instanceVertex.z, 1.0f);
		output.UV.x = instanceVertex.w;
		output.UV.y = input.UV[corner];

		if(input.Data.z == 4)
		{
			uint2 terrainPosition = GetTerrainVertexPosition(uint2(input.Size), corner);
			output.Color = float4(RequestGrassLight(terrainPosition.x, terrainPosition.y).xyz,
				input.Color.a);
		}
		else
		{
			output.Color = input.Color;
		}
	}
	else if(input.Data.z > 2)
	{
		output.Position = float4(instanceVertex.xyz, 1.0f);
		output.UV.x = instanceVertex.w;
		output.UV.y = input.UV[corner];
		output.Color = input.Color;

		if(selector.z <= 1.0f)
		{
			output.Color.xyz *= input.Size.x;
		}
		else
		{
			output.Color.xyz *= input.Size.y;
		}
	}
	else if(input.Data.z > 1)
	{
		output.Position = float4(input.Size.xxy * (selector.xxy * 2.0f), 1.0f);
		output.Position = TransformInstancePosition(input, output.Position);

		if(selector.x < 0.0f)
		{
			output.UV.x = input.UV.x;
		}
		else
		{
			output.UV.x = input.UV.z;
		}
	
		if(selector.y < 0.0f)
		{
			output.UV.y = input.UV.y;
		}
		else
		{
			output.UV.y = input.UV.w;
		}

		output.Color = input.Color;
	}
	else if(input.Data.z > 0)
	{
		output.Position = float4(instanceVertex.xyz, 1.0f);
		output.UV.x = instanceVertex.w;
		output.UV.y = input.UV[corner];
		output.Color = input.Color;
	}
	else
	{
		output.Position = float4(input.Size.x * selector.x,
								0.0f,
								input.Size.y * selector.y,
								1.0f);
		output.Position = TransformInstancePosition(input, output.Position);

		if(selector.x < 0.0f)
		{
			output.UV.x = input.UV.x;
		}
		else
		{
			output.UV.x = input.UV.z;
		}
	
		if(selector.y < 0.0f)
		{
			output.UV.y = input.UV.y;
		}
		else
		{
			output.UV.y = input.UV.w;
		}

		output.Color = input.Color;
	}
	output.Position = mul(Game3D, output.Position);
	
	output.Tex = float2(input.Data.xy);

	return output;
}
