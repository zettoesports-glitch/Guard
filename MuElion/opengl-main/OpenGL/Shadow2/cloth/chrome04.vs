// Generated from cloth/chrome04.vs. Edit the HLSL source and regenerate this file.

struct ClothVertexInput
{
    vec3 Position;
    vec2 UV;
    vec3 Normal;
    vec3 BodyOrigin;
    vec2 BodyScale;
    vec2 Data;
    vec4 Data2;
    vec4 BodyLight;
    vec4 ShadowLight;
    uint BoneIndex;
};

struct PixelInput
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

layout(location = 0) in vec3 input_Position;
layout(location = 1) in vec2 input_UV;
layout(location = 2) in vec3 input_Normal;
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

PixelInput _main(ClothVertexInput _input)
{
    PixelInput _output;
    _output.Position = vec4(_input.Position, 1.0) * spvWorkaroundRowMajor(GlobalConstantsData.Game3D);
    _output.Color = clamp(_input.BodyLight, vec4(0.0), vec4(1.0));
    float dotr = dot(_input.Normal, GlobalConstantsData.ChromeWave);
    vec2 uv = (_input.Normal.yz * 0.5) + vec2(GlobalConstantsData.ChromeWave.y * 3.0, GlobalConstantsData.Wave.x * 3.0);
    uv.y = -uv.y;
    _output.UV = (vec2(dotr, 1.0 - dotr) + uv) + _input.Data2.xy;
    _output.MinAlpha = _input.Data.y;
    return _output;
}

void main()
{
    ClothVertexInput _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.Normal = input_Normal;
    _input.BodyOrigin = input_BodyOrigin;
    _input.BodyScale = input_BodyScale;
    _input.Data = input_Data;
    _input.Data2 = input_Data2;
    _input.BodyLight = input_BodyLight;
    _input.ShadowLight = input_ShadowLight;
    _input.BoneIndex = input_BoneIndex;
    ClothVertexInput param = _input;
    PixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_Color = flattenTemp.Color;
    output_UV = flattenTemp.UV;
    output_MinAlpha = flattenTemp.MinAlpha;
}

