// Generated from shadows/shadowchrome04.vs. Edit the HLSL source and regenerate this file.

struct VertexInput
{
    vec3 Position;
    vec2 UV;
    vec3 Normal;
    uvec2 Bones;
    vec3 BodyOrigin;
    vec2 BodyScale;
    vec2 Data;
    vec4 Data2;
    vec4 BodyLight;
    vec4 ShadowLight;
    uint BoneIndex;
};

struct ShadowPixelInput
{
    vec4 Position;
    vec4 Color;
    vec2 UV;
    float MinAlpha;
};

layout(std140) uniform GlobalConstants
{
    vec4 ShadowColor;
    vec4 ShadowPosition;
    mat4 Shadow;
    mat4 Game3D;
    mat4 Game2D;
    vec4 BillboardQ;
    vec3 LightPosition;
    float WorldTime;
    vec3 ChromeWave;
    float Random;
    vec2 Wave;
    float WaterMove;
    float WindScale;
    float WindSpeed;
    int WaterIndex;
    vec2 Padding1;
} GlobalConstantsData;

uniform highp sampler2D BonesTexture;

layout(location = 0) in vec3 input_Position;
layout(location = 1) in vec2 input_UV;
layout(location = 2) in vec3 input_Normal;
layout(location = 3) in uvec2 input_Bones;
layout(location = 6) in vec3 input_BodyOrigin;
layout(location = 7) in vec2 input_BodyScale;
layout(location = 8) in vec2 input_Data;
layout(location = 9) in vec4 input_Data2;
layout(location = 10) in vec4 input_BodyLight;
layout(location = 11) in vec4 input_ShadowLight;
layout(location = 12) in uint input_BoneIndex;
layout(location = 0) out vec4 output_Color;
layout(location = 1) out vec2 output_UV;
layout(location = 2) flat out float output_MinAlpha;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

vec4 LoadBoneTexel(uint texelIndex)
{
    uint boneX = texelIndex % 1024u;
    uint boneY = texelIndex / 1024u;
    return texelFetch(BonesTexture, ivec3(uvec3(boneX, boneY, 0u)).xy, ivec3(uvec3(boneX, boneY, 0u)).z);
}

mat4 RequestBone(inout uint boneIndex, uint boneId)
{
    boneIndex += boneId;
    boneIndex *= 2u;
    uint param = boneIndex;
    vec4 rotation = LoadBoneTexel(param);
    uint param_1 = boneIndex + 1u;
    vec4 positionScale = LoadBoneTexel(param_1);
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
    return mat4(vec4(vec4((1.0 - (2.0 * (yy + zz))) * scale, (2.0 * (xy + wz)) * scale, (2.0 * (xz - wy)) * scale, 0.0)), vec4(vec4((2.0 * (xy - wz)) * scale, (1.0 - (2.0 * (xx + zz))) * scale, (2.0 * (yz + wx)) * scale, 0.0)), vec4(vec4((2.0 * (xz + wy)) * scale, (2.0 * (yz - wx)) * scale, (1.0 - (2.0 * (xx + yy))) * scale, 0.0)), vec4(vec4(positionScale.xyz, 1.0)));
}

ShadowPixelInput _main(VertexInput _input)
{
    ShadowPixelInput _output;
    if (_input.Data2.z == 0.0)
    {
        _output.Position = vec4(0.0);
        _output.Color = vec4(0.0);
        _output.UV = vec2(0.0);
        _output.MinAlpha = 1.0;
    }
    else
    {
        uint param = _input.BoneIndex;
        uint param_1 = _input.Bones.x;
        mat4 _251 = RequestBone(param, param_1);
        mat4 BoneMatrix = _251;
        _output.Position = BoneMatrix * vec4(_input.Position, 1.0);
        if (_input.Bones.x != _input.Bones.y)
        {
            uint param_2 = _input.BoneIndex;
            uint param_3 = _input.Bones.y;
            mat4 _275 = RequestBone(param_2, param_3);
            BoneMatrix = _275;
        }
        vec3 normal = mat3(BoneMatrix[0].xyz, BoneMatrix[1].xyz, BoneMatrix[2].xyz) * _input.Normal;
        _output.Position += (vec4(normal, 0.0) * _input.BodyScale.y);
        vec4 _305 = _output.Position;
        vec3 _307 = _305.xyz * _input.BodyScale.x;
        _output.Position.x = _307.x;
        _output.Position.y = _307.y;
        _output.Position.z = _307.z;
        vec4 _318 = _output.Position;
        vec3 _320 = _318.xyz + _input.BodyOrigin;
        _output.Position.x = _320.x;
        _output.Position.y = _320.y;
        _output.Position.z = _320.z;
        _output.Position *= spvWorkaroundRowMajor(GlobalConstantsData.Shadow);
        _output.Color = clamp(_input.BodyLight, vec4(0.0), vec4(1.0));
        float dotr = dot(normal, GlobalConstantsData.ChromeWave);
        vec2 uv = (normal.yz * 0.5) + vec2(GlobalConstantsData.ChromeWave.y * 3.0, GlobalConstantsData.Wave.x * 3.0);
        uv.y = -uv.y;
        _output.UV = (vec2(dotr, 1.0 - dotr) + uv) + _input.Data2.xy;
        _output.MinAlpha = _input.Data.y;
    }
    return _output;
}

void main()
{
    VertexInput _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.Normal = input_Normal;
    _input.Bones = input_Bones;
    _input.BodyOrigin = input_BodyOrigin;
    _input.BodyScale = input_BodyScale;
    _input.Data = input_Data;
    _input.Data2 = input_Data2;
    _input.BodyLight = input_BodyLight;
    _input.ShadowLight = input_ShadowLight;
    _input.BoneIndex = input_BoneIndex;
    VertexInput param = _input;
    ShadowPixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_Color = flattenTemp.Color;
    output_UV = flattenTemp.UV;
    output_MinAlpha = flattenTemp.MinAlpha;
}

