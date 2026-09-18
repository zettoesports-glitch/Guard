// Generated from terrains/terraingrass.vs. Edit the HLSL source and regenerate this file.

struct VertexInput
{
    vec3 Position;
    vec2 UV;
    vec2 T;
    vec4 Coords;
};

struct PixelInput
{
    vec4 Position;
    vec2 UV;
    float T;
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

uniform highp sampler2D HeightTexture;
uniform highp sampler2D LightTexture;

layout(location = 0) in vec3 input_Position;
layout(location = 1) in vec2 input_UV;
layout(location = 2) in vec2 input_T;
layout(location = 3) in vec4 input_Coords;
layout(location = 0) out vec2 output_UV;
layout(location = 1) flat out float output_T;
layout(location = 2) out vec4 output_Color;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

float RequestHeight(inout uint x, inout uint y)
{
    x &= 255u;
    y &= 255u;
    return texelFetch(HeightTexture, ivec3(uvec3(x, y, 0u)).xy, ivec3(uvec3(x, y, 0u)).z).x;
}

vec4 RequestGrassLight(uint X, uint Y)
{
    uint x = X % 256u;
    uint y = Y % 256u;
    return clamp(vec4(vec3(texelFetch(LightTexture, ivec3(int(x), int(y), 0).xy, ivec3(int(x), int(y), 0).z).xyz), 1.0), vec4(0.0), vec4(1.0));
}

PixelInput _main(VertexInput _input)
{
    PixelInput _output;
    _output.Position = vec4(_input.Position, 1.0);
    if (_input.T.x >= 0.0)
    {
        _output.Position.y += (sin(GlobalConstantsData.WindSpeed + (_input.T.x * 5.0)) * GlobalConstantsData.WindScale);
    }
    uint param = uint(_input.Coords.x);
    uint param_1 = uint(_input.Coords.y);
    float _147 = RequestHeight(param, param_1);
    _output.Position.z += _147;
    _output.UV = _input.UV;
    _output.T = _input.T.y;
    uint param_2 = uint(_input.Coords.z);
    uint param_3 = uint(_input.Coords.w);
    _output.Color = RequestGrassLight(param_2, param_3);
    _output.Position *= spvWorkaroundRowMajor(GlobalConstantsData.Game3D);
    return _output;
}

void main()
{
    VertexInput _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.T = input_T;
    _input.Coords = input_Coords;
    VertexInput param = _input;
    PixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_UV = flattenTemp.UV;
    output_T = flattenTemp.T;
    output_Color = flattenTemp.Color;
}

