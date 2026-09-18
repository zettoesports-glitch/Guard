// Generated from terrains/kalimawater.vs. Edit the HLSL source and regenerate this file.

struct VertexInput
{
    vec4 Position;
    vec3 Normal;
};

struct PixelInput
{
    vec4 Position;
    vec2 UV;
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

layout(location = 0) in vec4 input_Position;
layout(location = 1) in vec3 input_Normal;
layout(location = 0) out vec2 output_UV;
vec3 TransformVector;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

PixelInput _main(VertexInput _input)
{
    PixelInput _output;
    _output.Position = _input.Position * spvWorkaroundRowMajor(GlobalConstantsData.Game3D);
    _output.UV = (_input.Normal.zy * 0.5) + vec2(0.100000001490116119384765625, 0.5);
    return _output;
}

void main()
{
    TransformVector = vec3(1.0, -1.0, 1.0);
    VertexInput _input;
    _input.Position = input_Position;
    _input.Normal = input_Normal;
    VertexInput param = _input;
    PixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_UV = flattenTemp.UV;
}

