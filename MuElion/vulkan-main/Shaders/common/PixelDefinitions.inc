struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD0;
	nointerpolation float MinAlpha : TEXCOORD1;
};

struct ColorPixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	nointerpolation float MinAlpha : TEXCOORD1;
};

struct ShadowPixelInput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
#if SHADOW_TYPE <= 2
	float4 Color2 : COLOR1;
#endif
	float2 UV : TEXCOORD0;
	nointerpolation float MinAlpha : TEXCOORD1;
};