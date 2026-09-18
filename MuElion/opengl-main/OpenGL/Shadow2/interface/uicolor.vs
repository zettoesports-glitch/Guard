// Generated from interface/uicolor.vs. Edit the HLSL source and regenerate this file.

struct VertexInputType
{
    vec2 Position;
    vec2 UV;
    vec4 Color;
};

struct PixelOutputType
{
    vec4 Position;
    vec4 Color;
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

layout(location = 0) in vec2 input_Position;
layout(location = 1) in vec2 input_UV;
layout(location = 2) in vec4 input_Color;
layout(location = 0) out vec4 output_Color;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

PixelOutputType _main(VertexInputType _input)
{
    PixelOutputType _output;
    _output.Position = vec4(_input.Position, 0.0, 1.0) * spvWorkaroundRowMajor(GlobalConstantsData.Game2D);
    _output.Color = _input.Color;
    return _output;
}

void main()
{
    VertexInputType _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.Color = input_Color;
    VertexInputType param = _input;
    PixelOutputType flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_Color = flattenTemp.Color;
}

