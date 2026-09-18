// Generated from shadowscloth/shadow.vs. Edit the HLSL source and regenerate this file.

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

ShadowPixelInput _main(ClothVertexInput _input)
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
        _output.Position = vec4(_input.Position, 1.0) * spvWorkaroundRowMajor(GlobalConstantsData.Shadow);
        _output.UV = _input.UV;
        _output.MinAlpha = _input.Data.y;
        if (_input.Data.x == 1.0)
        {
            float Luminosity = (dot(_input.Normal, GlobalConstantsData.LightPosition) * 0.800000011920928955078125) + 0.4000000059604644775390625;
            if (Luminosity < 0.20000000298023223876953125)
            {
                Luminosity = 0.20000000298023223876953125;
            }
            _output.Color = clamp(_input.BodyLight * vec4(Luminosity, Luminosity, Luminosity, 1.0), vec4(0.0), vec4(1.0));
        }
        else
        {
            _output.Color = clamp(_input.BodyLight, vec4(0.0), vec4(1.0));
        }
    }
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
    ShadowPixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_Color = flattenTemp.Color;
    output_UV = flattenTemp.UV;
    output_MinAlpha = flattenTemp.MinAlpha;
}

