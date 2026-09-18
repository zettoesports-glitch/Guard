#define TERRAIN_SIZE_MASK 255
#define TERRAIN_SIZE 256
#define TERRAIN_SCALE 100.0f

#define TATTR_NOGROUND (0x0008)

Texture2D<float3> LightTexture : register(t0);
Texture2D<float> AlphaTexture : register(t1);
Texture2D<float> HeightTexture : register(t2);
Texture2D<uint> TerrainAttribute : register(t3);

float4 RequestLight(uint X, uint Y)
{
	uint x = X % TERRAIN_SIZE;
	uint y = Y % TERRAIN_SIZE;
	return saturate(float4(LightTexture.Load(int3(x, y, 0)),
						   AlphaTexture.Load(int3(x, y, 0))));
}

float4 RequestGrassLight(uint X, uint Y)
{
	uint x = X % TERRAIN_SIZE;
	uint y = Y % TERRAIN_SIZE;
	return saturate(float4(LightTexture.Load(int3(x, y, 0)), 1.0f));
}

float RequestHeight(uint x, uint y)
{
	x &= TERRAIN_SIZE_MASK;
	y &= TERRAIN_SIZE_MASK;

	return HeightTexture.Load(uint3(x, y, 0));
}

bool IsVisible(uint x, uint y)
{
	return (TerrainAttribute.Load(int3(x, y, 0)) & TATTR_NOGROUND) == 0;
}

uint2 GetTerrainVertexPosition(uint2 tilePosition, uint vertexIndex)
{
	uint corner = vertexIndex & 3u;
	if(corner == 1u)
		tilePosition.x += 1u;
	else if(corner == 2u)
		tilePosition += uint2(1u, 1u);
	else if(corner == 3u)
		tilePosition.y += 1u;

	return tilePosition;
}
