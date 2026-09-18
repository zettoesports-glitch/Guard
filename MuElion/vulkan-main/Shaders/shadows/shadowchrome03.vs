#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"
#include "SkeletonHelpers.inc"

#if SHADOW_TYPE <= 2
#include "TerrainModelHelpers.inc"
#endif

static const float3 TransformVector = { 0.0f, -0.1f, -0.8f };

ShadowPixelInput main(VertexInput input)
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
		matrix BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.x));
		output.Position = mul(float4(input.Position, 1.0f), BoneMatrix);
		if(input.Bones.x != input.Bones.y)
		{
			BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.y));
		}
		float3 normal = mul(input.Normal, (float3x3)BoneMatrix);
		
		output.Position += float4(normal, 0.0f) * input.BodyScale.y;

		output.Position.xyz *= input.BodyScale.x;
	output.Position.xyz += input.BodyOrigin;

		output.Position = mul(Shadow, output.Position);
		
		output.Color = saturate(input.BodyLight);
		float dotr = dot(normal, TransformVector);
		output.UV = float2(dotr, 1.0f - dotr);
		output.MinAlpha = input.Data.y;
#else
		float4 bodyOrigin = float4(input.BodyOrigin.xy, input.Data2.w, 0.0f);
		
		matrix BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.x));
		output.Position = mul(float4(input.Position, 1.0f), BoneMatrix);
		if(input.Bones.x != input.Bones.y)
		{
			BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.y));
		}
		float3 normal = mul(input.Normal, (float3x3)BoneMatrix);
		
		output.Position += float4(normal, 0.0f) * input.BodyScale.y;

		output.Position.xyz *= input.BodyScale.x;
	output.Position.xyz += input.BodyOrigin;
		
#if SHADOW_TYPE == 2
		output.Position -= bodyOrigin;
		output.Position.x += output.Position.z * (output.Position.x + ShadowPosition.x) / (output.Position.z - ShadowPosition.y);
		output.Position += bodyOrigin;
		output.Position.z = GetTerrainHeight(output.Position.xy) + 6.0f;
#else
		output.Position -= bodyOrigin;
		output.Position.x += output.Position.z * (output.Position.x + ShadowPosition.x) / (output.Position.z - ShadowPosition.y);
		output.Position.z = 5.0f;
		output.Position += bodyOrigin;
#endif

		output.Position = mul(Game3D, output.Position);
		output.Color = saturate(input.BodyLight);
		output.Color2 = input.ShadowLight;
		float dotr = dot(normal, TransformVector);
		output.UV = float2(dotr, 1.0f - dotr);
		output.MinAlpha = input.Data.y;
#endif
	}

	return output;
}