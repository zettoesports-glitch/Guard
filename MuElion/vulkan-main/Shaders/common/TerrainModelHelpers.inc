#define TERRAIN_SIZE_MASK 255
Texture2D<float> HeightTexture : register(t1);

float GetTerrainHeight(float2 Pos)
{
	Pos /= 100.0f;
	
	int2 iPos = Pos;
	
	iPos.x &= TERRAIN_SIZE_MASK;
	iPos.y &= TERRAIN_SIZE_MASK;
	
	float2 diff = Pos - (float2)iPos;
	
	float h1 = HeightTexture.Load(uint3(iPos, 0));
	float h2 = HeightTexture.Load(uint3(iPos + int2(0, 1), 0));
	float h3 = HeightTexture.Load(uint3(iPos + int2(1, 0), 0));
	float h4 = HeightTexture.Load(uint3(iPos + int2(1, 1), 0));
	
	float left = h1 + (h2 - h1) * diff.y;
	float right = h3 + (h4 - h3) * diff.y;
	
	return left + (right-left) * diff.x;
}