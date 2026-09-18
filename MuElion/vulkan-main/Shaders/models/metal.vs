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
	
	output.Color = saturate(input.BodyLight);
	output.UV = normal.zy * 0.5f + float2(0.2f, 0.5f);
	output.MinAlpha = input.Data.y;

	return output;
}