struct VertexInput
{
	[[vk::location(0)]] float3 Position : POSITION;
	[[vk::location(1)]] float2 UV : TEXCOORD0;
	[[vk::location(2)]] float3 Normal : NORMAL;
	// Bones[x: vertex, y: normal]
	[[vk::location(3)]] uint2 Bones : TEXCOORD1;

	[[vk::location(6)]] float3 BodyOrigin : INSTANCE0;
	// BodyScale[x], BoneScale[y]. CPU already resolves translated/non-translated instances.
	[[vk::location(7)]] float2 BodyScale : INSTANCE1;
	// EnableLight[x], MinAlpha[y]
	[[vk::location(8)]] float2 Data : INSTANCE2;
	// BlendMeshTexCoordUV[xy], EnableShadow[z], ShadowHeight[w]
	[[vk::location(9)]] float4 Data2 : INSTANCE3;
	// BodyLight[xyz], Alpha[w]
	[[vk::location(10)]] float4 BodyLight : INSTANCE4;
	[[vk::location(11)]] float4 ShadowLight : INSTANCE5;
	[[vk::location(12)]] uint BoneIndex : INSTANCE6;
};

struct ClothVertexInput
{
	[[vk::location(0)]] float3 Position : POSITION;
	[[vk::location(1)]] float2 UV : TEXCOORD0;
	[[vk::location(2)]] float3 Normal : NORMAL;

	[[vk::location(6)]] float3 BodyOrigin : INSTANCE0;
	[[vk::location(7)]] float2 BodyScale : INSTANCE1;
	// EnableLight[x], MinAlpha[y]
	[[vk::location(8)]] float2 Data : INSTANCE2;
	[[vk::location(9)]] float4 Data2 : INSTANCE3;
	// BodyLight[xyz], Alpha[w]
	[[vk::location(10)]] float4 BodyLight : INSTANCE4;
	[[vk::location(11)]] float4 ShadowLight : INSTANCE5;
	[[vk::location(12)]] uint BoneIndex : INSTANCE6;
};
