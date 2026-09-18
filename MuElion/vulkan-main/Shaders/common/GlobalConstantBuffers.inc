#define DISABLED_SHADOW (0.0f)

cbuffer GlobalConstants : register(b0)
{
	float4 ShadowColor;
	float4 ShadowPosition;
	matrix Shadow;
	matrix Game3D;
	matrix Game2D;
	float4 BillboardQ;
	float3 LightPosition;
	float WorldTime;
	float3 ChromeWave;
	float Random;
	float2 Wave;
	float WaterMove;
	float WindScale;
	float WindSpeed;
	int WaterIndex;
	float2 Padding1;
};
