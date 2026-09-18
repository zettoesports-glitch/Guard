// Generated from effects/effects.vs. Edit the HLSL source and regenerate this file.

struct VertexInput
{
    uint VertexId;
    vec4 Instance0;
    vec4 Instance1;
    vec4 Instance2;
    vec4 Instance3;
    vec4 UV;
    vec2 Size;
    vec4 Color;
    uvec4 Data;
};

struct PixelInput
{
    vec4 Position;
    vec4 Color;
    vec2 UV;
    vec2 Tex;
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

uniform highp sampler2D LightTexture;

layout(location = 0) in vec4 input_Instance0;
layout(location = 1) in vec4 input_Instance1;
layout(location = 2) in vec4 input_Instance2;
layout(location = 3) in vec4 input_Instance3;
layout(location = 4) in vec4 input_UV;
layout(location = 5) in vec2 input_Size;
layout(location = 6) in vec4 input_Color;
layout(location = 7) in uvec4 input_Data;
layout(location = 0) out vec4 output_Color;
layout(location = 1) out vec2 output_UV;
layout(location = 2) flat out vec2 output_Tex;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

vec3 RequestSelector(uint VertexId)
{
    switch (VertexId % 6u)
    {
        case 0u:
        {
            return vec3(-0.5, -0.5, 0.0);
        }
        case 1u:
        {
            return vec3(0.5, -0.5, 1.0);
        }
        case 2u:
        {
            return vec3(0.5, 0.5, 2.0);
        }
        case 3u:
        {
            return vec3(-0.5, -0.5, 0.0);
        }
        case 4u:
        {
            return vec3(0.5, 0.5, 2.0);
        }
        default:
        {
            return vec3(-0.5, 0.5, 3.0);
        }
    }
}

vec4 RequestInstanceColumn(VertexInput _input, uint Index)
{
    switch (Index)
    {
        case 0u:
        {
            return _input.Instance0;
        }
        case 1u:
        {
            return _input.Instance1;
        }
        case 2u:
        {
            return _input.Instance2;
        }
        default:
        {
            return _input.Instance3;
        }
    }
}

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

vec4 RequestGrassLight(uint X, uint Y)
{
    uint x = X % 256u;
    uint y = Y % 256u;
    return clamp(vec4(vec3(texelFetch(LightTexture, ivec3(int(x), int(y), 0).xy, ivec3(int(x), int(y), 0).z).xyz), 1.0), vec4(0.0), vec4(1.0));
}

vec4 TransformInstancePosition(VertexInput _input, vec4 Position)
{
    return (((_input.Instance0 * Position.x) + (_input.Instance1 * Position.y)) + (_input.Instance2 * Position.z)) + (_input.Instance3 * Position.w);
}

PixelInput _main(VertexInput _input)
{
    PixelInput _output;
    _output.UV = vec2(0.0);
    uint param = _input.VertexId;
    vec3 selector = RequestSelector(param);
    uint corner = uint(selector.z);
    VertexInput param_1 = _input;
    uint param_2 = corner;
    vec4 instanceVertex = RequestInstanceColumn(param_1, param_2);
    if ((_input.Data.z == 4u) || (_input.Data.z == 5u))
    {
        VertexInput param_3 = _input;
        uint param_4 = 0u;
        vec4 instanceOrigin = RequestInstanceColumn(param_3, param_4);
        vec2 terrainOffset = vec2((selector.x > 0.0) ? 100.0 : 0.0, (selector.y > 0.0) ? 100.0 : 0.0);
        _output.Position = vec4(instanceOrigin.xy + terrainOffset, instanceVertex.z, 1.0);
        _output.UV.x = instanceVertex.w;
        _output.UV.y = _input.UV[corner];
        if (_input.Data.z == 4u)
        {
            uvec2 param_5 = uvec2(_input.Size);
            uint param_6 = corner;
            uvec2 _282 = GetTerrainVertexPosition(param_5, param_6);
            uvec2 terrainPosition = _282;
            uint param_7 = terrainPosition.x;
            uint param_8 = terrainPosition.y;
            _output.Color = vec4(RequestGrassLight(param_7, param_8).xyz, _input.Color.w);
        }
        else
        {
            _output.Color = _input.Color;
        }
    }
    else
    {
        if (_input.Data.z > 2u)
        {
            _output.Position = vec4(instanceVertex.xyz, 1.0);
            _output.UV.x = instanceVertex.w;
            _output.UV.y = _input.UV[corner];
            _output.Color = _input.Color;
            if (selector.z <= 1.0)
            {
                vec4 _334 = _output.Color;
                vec3 _336 = _334.xyz * _input.Size.x;
                _output.Color.x = _336.x;
                _output.Color.y = _336.y;
                _output.Color.z = _336.z;
            }
            else
            {
                vec4 _347 = _output.Color;
                vec3 _349 = _347.xyz * _input.Size.y;
                _output.Color.x = _349.x;
                _output.Color.y = _349.y;
                _output.Color.z = _349.z;
            }
        }
        else
        {
            if (_input.Data.z > 1u)
            {
                _output.Position = vec4(_input.Size.xxy * (selector.xxy * 2.0), 1.0);
                VertexInput param_9 = _input;
                vec4 param_10 = _output.Position;
                _output.Position = TransformInstancePosition(param_9, param_10);
                if (selector.x < 0.0)
                {
                    _output.UV.x = _input.UV.x;
                }
                else
                {
                    _output.UV.x = _input.UV.z;
                }
                if (selector.y < 0.0)
                {
                    _output.UV.y = _input.UV.y;
                }
                else
                {
                    _output.UV.y = _input.UV.w;
                }
                _output.Color = _input.Color;
            }
            else
            {
                if (_input.Data.z > 0u)
                {
                    _output.Position = vec4(instanceVertex.xyz, 1.0);
                    _output.UV.x = instanceVertex.w;
                    _output.UV.y = _input.UV[corner];
                    _output.Color = _input.Color;
                }
                else
                {
                    _output.Position = vec4(_input.Size.x * selector.x, 0.0, _input.Size.y * selector.y, 1.0);
                    VertexInput param_11 = _input;
                    vec4 param_12 = _output.Position;
                    _output.Position = TransformInstancePosition(param_11, param_12);
                    if (selector.x < 0.0)
                    {
                        _output.UV.x = _input.UV.x;
                    }
                    else
                    {
                        _output.UV.x = _input.UV.z;
                    }
                    if (selector.y < 0.0)
                    {
                        _output.UV.y = _input.UV.y;
                    }
                    else
                    {
                        _output.UV.y = _input.UV.w;
                    }
                    _output.Color = _input.Color;
                }
            }
        }
    }
    _output.Position *= spvWorkaroundRowMajor(GlobalConstantsData.Game3D);
    _output.Tex = vec2(_input.Data.xy);
    return _output;
}

void main()
{
    VertexInput _input;
    _input.VertexId = uint(gl_VertexID);
    _input.Instance0 = input_Instance0;
    _input.Instance1 = input_Instance1;
    _input.Instance2 = input_Instance2;
    _input.Instance3 = input_Instance3;
    _input.UV = input_UV;
    _input.Size = input_Size;
    _input.Color = input_Color;
    _input.Data = input_Data;
    VertexInput param = _input;
    PixelInput flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_Color = flattenTemp.Color;
    output_UV = flattenTemp.UV;
    output_Tex = flattenTemp.Tex;
}

