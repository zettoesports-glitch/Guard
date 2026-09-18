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
	float dotr = dot(normal, ChromeWave);
	float2 uv = normal.yz * 0.5f + float2(ChromeWave.y * 3.0f, Wave.x * 3.0f);
	uv.y = -uv.y;
	output.UV = (float2(dotr, 1.0f - dotr) + uv) + input.Data2.xy;
	output.MinAlpha = input.Data.y;

	return output;
}