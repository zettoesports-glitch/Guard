#if USE_TEXTURE_BUFFER == 1
Buffer<float4> BonesTexture : register(t0);
#else
Texture2D<float4> BonesTexture : register(t0);
#endif

uint3 GetBoneIndex(uint baseX, uint baseY, uint matrixIndex)
{
	uint t = (baseX + matrixIndex);
	baseX = t % SKELETON_TEXTURE_WIDTH;
	baseY += t / SKELETON_TEXTURE_WIDTH;

	return uint3(baseX, baseY, 0);
}

float4 LoadBoneTexel(uint texelIndex)
{
#if USE_TEXTURE_BUFFER == 1
	return BonesTexture.Load(texelIndex);
#else
	uint boneX = texelIndex % SKELETON_TEXTURE_WIDTH;
	uint boneY = texelIndex / SKELETON_TEXTURE_WIDTH;
	return BonesTexture.Load(uint3(boneX, boneY, 0));
#endif
}

matrix RequestBone(uint boneIndex, uint boneId)
{
	boneIndex += boneId;

#if USE_DUAL_QUATERNION == 1
	// EDualQuat is stored as quaternion (x, y, z, w) followed by
	// position (x, y, z) and uniform scale. Build the transposed affine
	// matrix expected by the shaders' row-vector mul(position, matrix).
	boneIndex *= 2;

	float4 rotation = LoadBoneTexel(boneIndex);
	float4 positionScale = LoadBoneTexel(boneIndex + 1);
	float x = rotation.x;
	float y = rotation.y;
	float z = rotation.z;
	float w = rotation.w;
	float scale = positionScale.w;

	float xx = x * x;
	float yy = y * y;
	float zz = z * z;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	return matrix(
		float4((1.0f - 2.0f * (yy + zz)) * scale,
			   (2.0f * (xy + wz)) * scale,
			   (2.0f * (xz - wy)) * scale, 0.0f),
		float4((2.0f * (xy - wz)) * scale,
			   (1.0f - 2.0f * (xx + zz)) * scale,
			   (2.0f * (yz + wx)) * scale, 0.0f),
		float4((2.0f * (xz + wy)) * scale,
			   (2.0f * (yz - wx)) * scale,
			   (1.0f - 2.0f * (xx + yy)) * scale, 0.0f),
		float4(positionScale.xyz, 1.0f));
#else
	boneIndex *= 4;

	return matrix(LoadBoneTexel(boneIndex),
				  LoadBoneTexel(boneIndex + 1),
				  LoadBoneTexel(boneIndex + 2),
				  LoadBoneTexel(boneIndex + 3));
#endif
}
