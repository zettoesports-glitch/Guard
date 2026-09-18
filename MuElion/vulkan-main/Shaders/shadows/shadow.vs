#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"
#include "SkeletonHelpers.inc"

#if SHADOW_TYPE <= 2
#include "TerrainModelHelpers.inc"
#endif

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

		output.UV = input.UV + input.Data2.xy;
		output.MinAlpha = input.Data.y;

		if(input.Data.x == 1.0f)
		{
			float Luminosity = dot(normal, LightPosition) * 0.8f + 0.4f;

			if(Luminosity < 0.2f)
			{
				Luminosity = 0.2f;
			}

			output.Color = saturate(input.BodyLight * float4(Luminosity, Luminosity, Luminosity, 1.0f));
		}
		else
		{
			output.Color = saturate(input.BodyLight);
		}
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
		
		output.Color2 = input.ShadowLight;
		output.UV = input.UV + input.Data2.xy;
		output.MinAlpha = input.Data.y;

		if(input.Data.x == 1.0f)
		{
			float Luminosity = dot(normal, LightPosition) * 0.8f + 0.4f;

			if(Luminosity < 0.2f)
			{
				Luminosity = 0.2f;
			}

			output.Color = saturate(input.BodyLight * float4(Luminosity, Luminosity, Luminosity, 1.0f));
		}
		else
		{
			output.Color = saturate(input.BodyLight);
		}
#endif
	}

	return output;
}