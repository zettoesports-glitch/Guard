#include "GlobalConstantBuffers.inc"
#include "VertexDefinitions.inc"
#include "PixelDefinitions.inc"
#include "SkeletonHelpers.inc"

ColorPixelInput main(VertexInput input)
{
	ColorPixelInput output;
	
	matrix BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.x));
	output.Position = mul(float4(input.Position, 1.0f), BoneMatrix);
	if(input.Bones.x != input.Bones.y)
	{
		BoneMatrix = RequestBone(uint(input.BoneIndex), uint(input.Bones.y));
	}
	float4 Normal = mul(float4(input.Normal, 0.0f), BoneMatrix);
	
	output.Position += Normal * input.BodyScale.y;

	output.Position.xyz *= input.BodyScale.x;
	output.Position.xyz += input.BodyOrigin;

	output.Position = mul(Game3D, output.Position);
	
	output.Color = saturate(input.BodyLight);
	output.MinAlpha = input.Data.y;

	return output;
}