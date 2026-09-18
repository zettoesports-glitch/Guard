// Generated from fonts/normalfont.vs. Edit the HLSL source and regenerate this file.

struct VertexInputType
{
    vec4 Position;
    vec4 UV;
    vec4 Color;
    uint TextureIndex;
    uint VertexId;
};

struct PixelInputType
{
    vec4 Position;
    vec2 UV;
    uint Texture;
    vec4 Color;
};

layout(std140) uniform FontsConstants
{
    mat4 TextMatrix;
    vec4 FontSettings;
    vec4 OutlineColor;
} FontsConstantsData;

layout(location = 0) in vec4 input_Position;
layout(location = 1) in vec4 input_UV;
layout(location = 2) in vec4 input_Color;
layout(location = 3) in uint input_TextureIndex;
layout(location = 0) out vec2 output_UV;
layout(location = 1) flat out uint output_Texture;
layout(location = 2) out vec4 output_Color;

highp mat4 spvWorkaroundRowMajor(highp mat4 wrap) { return transpose(wrap); }
mediump mat4 spvWorkaroundRowMajorMP(mediump mat4 wrap) { return transpose(wrap); }

PixelInputType _main(VertexInputType _input)
{
    vec2 position;
    vec2 uv;
    switch (_input.VertexId % 6u)
    {
        case 0u:
        {
            position = vec2(_input.Position.x, _input.Position.z);
            uv = vec2(_input.UV.x, _input.UV.z);
            break;
        }
        case 1u:
        {
            position = vec2(_input.Position.y, _input.Position.z);
            uv = vec2(_input.UV.y, _input.UV.z);
            break;
        }
        case 2u:
        case 3u:
        {
            position = vec2(_input.Position.x, _input.Position.w);
            uv = vec2(_input.UV.x, _input.UV.w);
            break;
        }
        case 4u:
        {
            position = vec2(_input.Position.y, _input.Position.z);
            uv = vec2(_input.UV.y, _input.UV.z);
            break;
        }
        default:
        {
            position = vec2(_input.Position.y, _input.Position.w);
            uv = vec2(_input.UV.y, _input.UV.w);
            break;
        }
    }
    PixelInputType _output;
    if (FontsConstantsData.FontSettings.x > 0.0)
    {
        _output.Position = vec4(position, 0.0, 1.0) * spvWorkaroundRowMajor(FontsConstantsData.TextMatrix);
    }
    else
    {
        _output.Position = vec4(position, 0.0, 1.0) * spvWorkaroundRowMajor(FontsConstantsData.TextMatrix);
    }
    _output.UV = uv;
    _output.Texture = _input.TextureIndex;
    _output.Color = _input.Color;
    return _output;
}

void main()
{
    VertexInputType _input;
    _input.Position = input_Position;
    _input.UV = input_UV;
    _input.Color = input_Color;
    _input.TextureIndex = input_TextureIndex;
    _input.VertexId = uint(gl_VertexID);
    VertexInputType param = _input;
    PixelInputType flattenTemp = _main(param);
    gl_Position = flattenTemp.Position;
    output_UV = flattenTemp.UV;
    output_Texture = flattenTemp.Texture;
    output_Color = flattenTemp.Color;
}

