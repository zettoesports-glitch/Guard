cbuffer LocalConstants : register(b1)
{
	matrix TextMatrix;
	float Is3D;
	float3 Padding;
	float4 OutlineColor;
};

struct VertexInputType
{
	float4 Position : POSITION;
	float4 UV : TEXCOORD0;
	float4 Color : COLOR0;
	uint TextureIndex : TEXCOORD1;
	uint VertexId : SV_VertexID;
};

struct PixelInputType
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
	nointerpolation uint Texture : TEXCOORD1;
	float4 Color : COLOR0;
};

PixelInputType main( VertexInputType input )
{
	PixelInputType output;
	float2 position;
	float2 uv;

	switch (input.VertexId % 6u)
	{
	case 0u:
		position = float2(input.Position.x, input.Position.z);
		uv = float2(input.UV.x, input.UV.z);
		break;
	case 1u:
		position = float2(input.Position.y, input.Position.z);
		uv = float2(input.UV.y, input.UV.z);
		break;
	case 2u:
	case 3u:
		position = float2(input.Position.x, input.Position.w);
		uv = float2(input.UV.x, input.UV.w);
		break;
	case 4u:
		position = float2(input.Position.y, input.Position.z);
		uv = float2(input.UV.y, input.UV.z);
		break;
	default:
		position = float2(input.Position.y, input.Position.w);
		uv = float2(input.UV.y, input.UV.w);
		break;
	}

	if(Is3D > 0.0f)
	{
		output.Position = mul(TextMatrix, float4(position, 0.0f, 1.0f));
	}
	else
	{
		output.Position = mul(TextMatrix, float4(position, 0.0f, 1.0f));
		output.Position.y = -output.Position.y;
	}

	output.UV = uv;
	output.Texture = input.TextureIndex;
	output.Color = input.Color;

	return output;
}
