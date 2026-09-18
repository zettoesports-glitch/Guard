// Generated from terrains/terrainatlans.vs. Edit the HLSL source and regenerate this file.

struct VertexInput
{
    uvec4 Position;
    vec2 UV;
    uint VertexIndex;
};

struct PixelInput
{
    vec4 Position;
    vec2 UV1;
    vec2 UV2;
    vec2 W;
    float Water;
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

uniform highp usampler2D AttributeTexture;
uniform highp sampler2D LightTexture;
uniform highp sampler2D AlphaTexture;
uniform highp sampler2D HeightTexture;

layout(location = 0) in uvec4 input_Position;
layout(location = 1) in vec2 input_UV;
layout(location = 0) out vec2 output_UV1;
layout(location = 1) out vec2 output_UV2;
layout(location = 2) flat out vec2 output_W;
layout(location = 3) flat out float output_Water;
layout(location = 4) out vec4 output_Color;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

uvec2 GetTerrainVertexPosition(inout uvec2 tilePosition, uint vertexIndex)
{
    uint corner = vertexIndex & 3u;
    if (corner == 1u)
    {
        tilePosition.x++;
    }
    else
    {
        if (corner == 2u)
        {
            tilePosition += uvec2(1u);
        }
        else
        {
            if (corner == 3u)
            {
                tilePosition.y++;
            }
        }
    }
    return tilePosition;
}

bool IsVisible(uint x, uint y)
{
    return (texelFetch(AttributeTexture, ivec3(int(x), int(y), 0).xy, ivec3(int(x), int(y), 0).z).x & 8u) == 0u;
}

vec4 RequestLight(uint X, uint Y)
{
    uint x = X % 256u;
    uint y = Y % 256u;
    return clamp(vec4(vec3(texelFetch(LightTexture, ivec3(int(x), int(y), 0).xy, ivec3(int(x), int(y), 0).z).xyz), texelFetch(AlphaTexture, ivec3(int(x), int(y), 0).xy, ivec3(int(x), int(y), 0).z).x), vec4(0.0), vec4(1.0));
}

float RequestHeight(inout uint x, inout uint y)
{
    x &= 255u;
    y &= 255u;
    return texelFetch(HeightTexture, ivec3(uvec3(x, y, 0u)).xy, ivec3(uvec3(x, y, 0u)).z).x;
}

PixelInput _main(VertexInput _input)
{
    vec2 mapping = vec2(float(_input.Position.z), float(_input.Position.w));
    uvec2 param = _input.Position.xy;
    uint param_1 = _input.VertexIndex;
    uvec2 _201 = GetTerrainVertexPosition(param, param_1);
    uvec2 terrainPosition = _201;
    uint param_2 = _input.Position.x;
    uint param_3 = _input.Position.y;
    PixelInput _output;
    if (IsVisible(param_2, param_3) == true)
    {
        uint param_4 = terrainPosition.x;
        uint param_5 = terrainPosition.y;
        _output.Color = RequestLight(param_4, param_5);
        _output.W = mapping;
    }
    else
    {
        _output.Color = vec4(0.0);
        _output.W = vec2(-1.0);
    }
    uint param_6 = terrainPosition.x;
    uint param_7 = terrainPosition.y;
    float _245 = RequestHeight(param_6, param_7);
    vec4 worldPos = vec4(vec2(terrainPosition) * 100.0, _245, 1.0);
    _output.Position = worldPos * spvWorkaroundRowMajor(GlobalConstantsData.Game3D);
    if (mapping.x == float(GlobalConstantsData.WaterIndex))
    {
        float GrassWind = (sin(GlobalConstantsData.WindSpeed + (float(terrainPosition.x) * 5.0)) * GlobalConstantsData.WindScale) * 0.00200000009499490261077880859375;
        _output.UV1 = vec2((float(terrainPosition.x) * _input.UV.x) + GlobalConstantsData.WaterMove, (float(terrainPosition.y) * _input.UV.x) + GrassWind);
        if (mapping.y == float(GlobalConstantsData.WaterIndex))
        {
            _output.UV2 = vec2((float(terrainPosition.x) * (_input.UV.y / 4.0)) + GlobalConstantsData.WaterMove, (float(terrainPosition.y) * (_input.UV.y / 4.0)) + GrassWind);
            _output.Water = 1.0;
        }
        else
        {
            _output.UV2 = vec2(float(terrainPosition.x) * _input.UV.y, float(terrainPosition.y) * _input.UV.y);
            _output.Water = 0.0;
        }
    }
    else
    {
        _output.UV1 = vec2(float(terrainPosition.x) * _input.UV.x, float(terrainPosition.y) * _input.UV.x);
        _output.UV2 = vec2(float(terrainPosition.x) * _input.UV.y, float(terrainPosition.y) * _input.UV.y);
        _output.Water = 0.0;
    }
    return _output;
}

void main()
{
    VertexInput _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.VertexIndex = uint(gl_VertexID);
    VertexInput param = _input;
    PixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_UV1 = flattenTemp.UV1;
    output_UV2 = flattenTemp.UV2;
    output_W = flattenTemp.W;
    output_Water = flattenTemp.Water;
    output_Color = flattenTemp.Color;
}

