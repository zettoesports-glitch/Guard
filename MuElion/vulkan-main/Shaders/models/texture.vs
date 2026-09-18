#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"
#include "SkeletonHelpers.inc"

PixelInput main(VertexInput input)
{
	PixelInput output;
	
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

	output.Position = mul(Game3D, output.Position);

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

	return output;
}