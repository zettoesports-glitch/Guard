#include "Render/Renderer/GLCompatShim.h"

#include "Render/Renderer/MuRenderer.h"
#include "client/render/LegacyRenderFacade.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
constexpr MUCompatGLenum kGLDepthTest = 0x0B71;
constexpr MUCompatGLenum kGLAlphaTest = 0x0BC0;
constexpr MUCompatGLenum kGLTexture2D = 0x0DE1;
constexpr MUCompatGLenum kGLBlend = 0x0BE2;
constexpr MUCompatGLenum kGLCullFace = 0x0B44;
constexpr MUCompatGLenum kGLFog = 0x0B60;
constexpr MUCompatGLenum kGLStencilTest = 0x0B90;
constexpr MUCompatGLenum kGLQuads = 0x0007;
constexpr MUCompatGLenum kGLTriangles = 0x0004;
constexpr MUCompatGLenum kGLLines = 0x0001;
constexpr MUCompatGLenum kGLTriangleFan = 0x0006;
constexpr MUCompatGLenum kGLQuadStrip = 0x0008;
constexpr MUCompatGLenum kGLModelViewMatrix = 0x0BA6;
constexpr MUCompatGLenum kGLProjectionMatrix = 0x0BA7;
constexpr MUCompatGLenum kGLSrcAlpha = 0x0302;
constexpr MUCompatGLenum kGLOneMinusSrcAlpha = 0x0303;
constexpr MUCompatGLenum kGLOne = 1;
constexpr MUCompatGLenum kGLZero = 0;
constexpr MUCompatGLenum kGLOneMinusSrcColor = 0x0301;
constexpr MUCompatGLenum kGLOneMinusDstColor = 0x0306;
constexpr MUCompatGLenum kGLSrcColor = 0x0300;
constexpr MUCompatGLenum kGLUnsignedByte = 0x1401;
constexpr MUCompatGLenum kGLRgb = 0x1907;
constexpr MUCompatGLenum kGLRgba = 0x1908;
constexpr MUCompatGLenum kGLFloat = 0x1406;
constexpr MUCompatGLenum kGLVertexArray = 0x8074;
constexpr MUCompatGLenum kGLColorArray = 0x8076;
constexpr MUCompatGLenum kGLTextureCoordArray = 0x8078;
constexpr MUCompatGLenum kGLFogMode = 0x0B65;
constexpr MUCompatGLenum kGLFogDensity = 0x0B62;
constexpr MUCompatGLenum kGLFogStart = 0x0B63;
constexpr MUCompatGLenum kGLFogEnd = 0x0B64;
constexpr MUCompatGLenum kGLFogColor = 0x0B66;

struct ImmediateVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float nx = 0.0f;
    float ny = 0.0f;
    float nz = 1.0f;
    float u = 0.0f;
    float v = 0.0f;
    std::uint32_t color = 0xFFFFFFFFu;
};

struct ClientPointerState
{
    MUCompatGLint size = 0;
    MUCompatGLenum type = 0;
    MUCompatGLsizei stride = 0;
    const void* pointer = nullptr;
    bool enabled = false;
};

MUCompatGLenum s_mode = 0;
std::vector<ImmediateVertex> s_vertices;
std::vector<mu::Vertex3D> s_renderVertices;
float s_u = 0.0f;
float s_v = 0.0f;
float s_nx = 0.0f;
float s_ny = 0.0f;
float s_nz = 1.0f;
std::uint32_t s_color = 0xFFFFFFFFu;
std::uint32_t s_boundTexture = 0;
constexpr MUCompatGLuint kFirstDynamicTextureId = 0x40000000u;
constexpr MUCompatGLuint kLastDynamicTextureId = 0x7FFFFFFFu;
MUCompatGLuint s_nextTexture = kFirstDynamicTextureId;
bool s_depthTest = false;
bool s_texture2D = true;
bool s_blend = false;
bool s_alphaTest = false;
bool s_cullFace = false;
bool s_fog = false;
bool s_stencil = false;
ClientPointerState s_vertexPointer;
ClientPointerState s_colorPointer;
ClientPointerState s_texCoordPointer;
mu::FogParams s_fogParams{static_cast<int>(0x2601), 0.0f, 1.0f, 1.0f, {0.0f, 0.0f, 0.0f, 1.0f}};

[[nodiscard]] std::uint8_t ByteFromFloat(float value)
{
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

[[nodiscard]] std::uint32_t PackColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(r);
}

[[nodiscard]] MUCompatGLuint AllocateDynamicTextureId()
{
    const MUCompatGLuint firstCandidate = s_nextTexture;
    do
    {
        const MUCompatGLuint candidate = s_nextTexture;
        s_nextTexture = candidate == kLastDynamicTextureId ? kFirstDynamicTextureId : candidate + 1u;
        if (!mu::GetRenderer().IsTextureRegistered(candidate))
        {
            return candidate;
        }
    } while (s_nextTexture != firstCandidate);

    return 0u;
}

void PushVertex(float x, float y, float z)
{
    s_vertices.push_back({x, y, z, s_nx, s_ny, s_nz, s_u, s_v, s_color});
}

void SubmitTriangles(const std::vector<ImmediateVertex>& vertices)
{
    s_renderVertices.resize(vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        const ImmediateVertex& v = vertices[i];
        s_renderVertices[i] = {v.x, v.y, v.z, v.nx, v.ny, v.nz, v.u, v.v, v.color};
    }
    mu::GetRenderer().RenderTriangles(s_renderVertices, s_texture2D ? s_boundTexture : 0u);
}

void SubmitQuads()
{
    s_renderVertices.resize((s_vertices.size() / 4) * 4);
    std::size_t output = 0;
    for (std::size_t i = 0; i + 3 < s_vertices.size(); i += 4)
    {
        const ImmediateVertex* quad = s_vertices.data() + i;
        s_renderVertices[output++] = {quad[0].x, quad[0].y, quad[0].z, quad[0].nx, quad[0].ny, quad[0].nz, quad[0].u, quad[0].v, quad[0].color};
        s_renderVertices[output++] = {quad[1].x, quad[1].y, quad[1].z, quad[1].nx, quad[1].ny, quad[1].nz, quad[1].u, quad[1].v, quad[1].color};
        s_renderVertices[output++] = {quad[2].x, quad[2].y, quad[2].z, quad[2].nx, quad[2].ny, quad[2].nz, quad[2].u, quad[2].v, quad[2].color};
        s_renderVertices[output++] = {quad[3].x, quad[3].y, quad[3].z, quad[3].nx, quad[3].ny, quad[3].nz, quad[3].u, quad[3].v, quad[3].color};
    }
    mu::GetRenderer().RenderQuad3D(s_renderVertices, s_texture2D ? s_boundTexture : 0u);
}

void SubmitTriangleFan()
{
    std::vector<ImmediateVertex> tris;
    if (s_vertices.size() < 3)
    {
        return;
    }
    tris.reserve((s_vertices.size() - 2) * 3);
    for (std::size_t i = 1; i + 1 < s_vertices.size(); ++i)
    {
        tris.push_back(s_vertices[0]);
        tris.push_back(s_vertices[i]);
        tris.push_back(s_vertices[i + 1]);
    }
    SubmitTriangles(tris);
}

void ApplyBlend(MUCompatGLenum sfactor, MUCompatGLenum dfactor)
{
    if (sfactor == kGLSrcAlpha && dfactor == kGLOneMinusSrcAlpha)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Alpha);
    else if (sfactor == kGLSrcAlpha && dfactor == kGLOne)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Additive);
    else if (sfactor == kGLZero && dfactor == kGLOneMinusSrcColor)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Subtract);
    else if (sfactor == kGLOneMinusDstColor && dfactor == kGLZero)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::InverseColor);
    else if (sfactor == kGLOne && dfactor == kGLOneMinusSrcAlpha)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Mixed);
    else if (sfactor == kGLZero && dfactor == kGLSrcColor)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::LightMap);
    else if (sfactor == kGLOne && dfactor == kGLOne)
        mu::GetRenderer().SetBlendMode(mu::BlendMode::Glow);
}

[[nodiscard]] const void* PrepareTextureUpload(MUCompatGLenum format, MUCompatGLenum type, const void* pixels,
                                               MUCompatGLsizei width, MUCompatGLsizei height,
                                               std::vector<std::uint8_t>& rgbaScratch)
{
    if (!pixels || width <= 0 || height <= 0 || type != kGLUnsignedByte)
    {
        return pixels;
    }

    if (format == kGLRgba)
    {
        return pixels;
    }

    if (format != kGLRgb)
    {
        return pixels;
    }

    const auto pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const auto* src = static_cast<const std::uint8_t*>(pixels);
    rgbaScratch.resize(pixelCount * 4u);
    for (std::size_t i = 0; i < pixelCount; ++i)
    {
        rgbaScratch[i * 4u + 0u] = src[i * 3u + 0u];
        rgbaScratch[i * 4u + 1u] = src[i * 3u + 1u];
        rgbaScratch[i * 4u + 2u] = src[i * 3u + 2u];
        rgbaScratch[i * 4u + 3u] = 255u;
    }
    return rgbaScratch.data();
}
} // namespace

void mu_glBegin(MUCompatGLenum mode)
{
    s_mode = mode;
    s_vertices.clear();
    (void)mu::pipeline::GetLegacyRenderFacade().Begin(
        static_cast<mu::pipeline::LegacyPrimitive>(mode));
}

void mu_glEnd()
{
    (void)mu::pipeline::GetLegacyRenderFacade().End();
    s_mode = 0;
    s_vertices.clear();
}

void mu_glVertex2f(MUCompatGLfloat x, MUCompatGLfloat y)
{
    PushVertex(x, y, 0.0f);
    (void)mu::pipeline::GetLegacyRenderFacade().Vertex3(x, y, 0.0f);
}
void mu_glVertex3f(MUCompatGLfloat x, MUCompatGLfloat y, MUCompatGLfloat z)
{
    PushVertex(x, y, z);
    (void)mu::pipeline::GetLegacyRenderFacade().Vertex3(x, y, z);
}
void mu_glVertex3fv(const MUCompatGLfloat* v)
{
    if (!v) return;
    PushVertex(v[0], v[1], v[2]);
    (void)mu::pipeline::GetLegacyRenderFacade().Vertex3(v[0], v[1], v[2]);
}
void mu_glTexCoord2f(MUCompatGLfloat u, MUCompatGLfloat v)
{
    s_u = u; s_v = v;
    (void)mu::pipeline::GetLegacyRenderFacade().TexCoord2(u, v);
}
void mu_glTexCoord2fv(const MUCompatGLfloat* uv)
{
    if (!uv) return;
    mu_glTexCoord2f(uv[0], uv[1]);
}
void mu_glColor3f(MUCompatGLfloat r, MUCompatGLfloat g, MUCompatGLfloat b) { mu_glColor4f(r, g, b, 1.0f); }
void mu_glColor3fv(const MUCompatGLfloat* c) { if (c) mu_glColor4f(c[0], c[1], c[2], 1.0f); }
void mu_glColor4fv(const MUCompatGLfloat* c) { if (c) mu_glColor4f(c[0], c[1], c[2], c[3]); }
void mu_glColor3ub(MUCompatGLubyte r, MUCompatGLubyte g, MUCompatGLubyte b) { mu_glColor4ub(r, g, b, 255); }
void mu_glColor4f(MUCompatGLfloat r, MUCompatGLfloat g, MUCompatGLfloat b, MUCompatGLfloat a)
{
    s_color = PackColor(ByteFromFloat(r), ByteFromFloat(g), ByteFromFloat(b), ByteFromFloat(a));
    (void)mu::pipeline::GetLegacyRenderFacade().Color4(r, g, b, a);
}
void mu_glColor4ub(MUCompatGLubyte r, MUCompatGLubyte g, MUCompatGLubyte b, MUCompatGLubyte a)
{
    s_color = PackColor(r, g, b, a);
    constexpr float inv = 1.0f / 255.0f;
    (void)mu::pipeline::GetLegacyRenderFacade().Color4(
        static_cast<float>(r) * inv, static_cast<float>(g) * inv,
        static_cast<float>(b) * inv, static_cast<float>(a) * inv);
}

void mu_glEnable(MUCompatGLenum cap)
{
    auto& facade = mu::pipeline::GetLegacyRenderFacade();
    if (cap == kGLDepthTest) { s_depthTest = true; (void)facade.SetDepthTestEnable(true); }
    else if (cap == kGLTexture2D) { s_texture2D = true; (void)facade.SetTextureEnable(true); }
    else if (cap == kGLBlend) { s_blend = true; (void)facade.SetBlendEnable(true); }
    else if (cap == kGLAlphaTest) { s_alphaTest = true; (void)facade.SetAlphaTestEnable(true); }
    else if (cap == kGLCullFace) { s_cullFace = true; (void)facade.SetCullEnable(true); }
    else if (cap == kGLFog) { s_fog = true; (void)facade.SetFogEnable(true); }
    else if (cap == kGLStencilTest) { s_stencil = true; (void)facade.SetStencilEnable(true); }
}

void mu_glDisable(MUCompatGLenum cap)
{
    auto& facade = mu::pipeline::GetLegacyRenderFacade();
    if (cap == kGLDepthTest) { s_depthTest = false; (void)facade.SetDepthTestEnable(false); }
    else if (cap == kGLTexture2D) { s_texture2D = false; (void)facade.SetTextureEnable(false); }
    else if (cap == kGLBlend) { s_blend = false; (void)facade.SetBlendEnable(false); }
    else if (cap == kGLAlphaTest) { s_alphaTest = false; (void)facade.SetAlphaTestEnable(false); }
    else if (cap == kGLCullFace) { s_cullFace = false; (void)facade.SetCullEnable(false); }
    else if (cap == kGLFog) { s_fog = false; (void)facade.SetFogEnable(false); }
    else if (cap == kGLStencilTest) { s_stencil = false; (void)facade.SetStencilEnable(false); }
}

MUCompatGLboolean mu_glIsEnabled(MUCompatGLenum cap)
{
    if (cap == kGLDepthTest) return s_depthTest;
    if (cap == kGLTexture2D) return s_texture2D;
    if (cap == kGLBlend) return s_blend;
    if (cap == kGLAlphaTest) return s_alphaTest;
    if (cap == kGLCullFace) return s_cullFace;
    if (cap == kGLFog) return s_fog;
    if (cap == kGLStencilTest) return s_stencil;
    return 0;
}

void mu_glBlendFunc(MUCompatGLenum sfactor, MUCompatGLenum dfactor) { (void)mu::pipeline::GetLegacyRenderFacade().SetBlendFunc(static_cast<mu::pipeline::RenderBlendFactor>(sfactor), static_cast<mu::pipeline::RenderBlendFactor>(dfactor)); }
void mu_glClear(MUCompatGLbitfield mask) { (void)mu::pipeline::GetLegacyRenderFacade().Clear((mask & 0x00004000u) != 0, (mask & 0x00000100u) != 0, (mask & 0x00000400u) != 0); }
void mu_glClearColor(MUCompatGLfloat r, MUCompatGLfloat g, MUCompatGLfloat b, MUCompatGLfloat a) { (void)mu::pipeline::GetLegacyRenderFacade().SetClearColor({r,g,b,a}); }
void mu_glDepthFunc(MUCompatGLenum func) { (void)mu::pipeline::GetLegacyRenderFacade().SetDepthFunc(static_cast<mu::pipeline::RenderCompareFunction>(func)); }
void mu_glDepthMask(MUCompatGLboolean flag) { (void)mu::pipeline::GetLegacyRenderFacade().SetDepthWriteEnable(flag != 0); }
void mu_glMatrixMode(MUCompatGLenum mode) { (void)mu::pipeline::GetLegacyRenderFacade().MatrixMode(static_cast<mu::pipeline::LegacyMatrixMode>(mode)); }
void mu_glPushMatrix() { (void)mu::pipeline::GetLegacyRenderFacade().PushMatrix(); }
void mu_glPopMatrix() { (void)mu::pipeline::GetLegacyRenderFacade().PopMatrix(); }
void mu_glLoadIdentity() { (void)mu::pipeline::GetLegacyRenderFacade().LoadIdentity(); }
void mu_glLoadMatrixf(const MUCompatGLfloat* matrix) { if (matrix) { std::array<float,16> m{}; std::copy_n(matrix,16,m.begin()); (void)mu::pipeline::GetLegacyRenderFacade().LoadMatrix(m); } }
void mu_glMultMatrixf(const MUCompatGLfloat* matrix) { if (matrix) { std::array<float,16> m{}; std::copy_n(matrix,16,m.begin()); (void)mu::pipeline::GetLegacyRenderFacade().MultMatrix(m); } }
void mu_glTranslatef(MUCompatGLfloat x, MUCompatGLfloat y, MUCompatGLfloat z) { (void)mu::pipeline::GetLegacyRenderFacade().Translate(x, y, z); }
void mu_glRotatef(MUCompatGLfloat angle, MUCompatGLfloat x, MUCompatGLfloat y, MUCompatGLfloat z) { (void)mu::pipeline::GetLegacyRenderFacade().Rotate(angle, x, y, z); }
void mu_glScalef(MUCompatGLfloat x, MUCompatGLfloat y, MUCompatGLfloat z) { (void)mu::pipeline::GetLegacyRenderFacade().Scale(x, y, z); }
void mu_glGetFloatv(MUCompatGLenum pname, MUCompatGLfloat* data)
{
    if (pname == kGLModelViewMatrix || pname == kGLProjectionMatrix) mu::GetRenderer().GetMatrix(static_cast<int>(pname), data);
}
void mu_glGetIntegerv(MUCompatGLenum, MUCompatGLint* data)
{
    if (data != nullptr)
    {
        data[0] = 0;
        data[1] = 0;
    }
}
void mu_glViewport(MUCompatGLint x, MUCompatGLint y, MUCompatGLsizei width, MUCompatGLsizei height) { (void)mu::pipeline::GetLegacyRenderFacade().SetViewport({x,y,width,height}); }
void mu_glScissor(MUCompatGLint x, MUCompatGLint y, MUCompatGLsizei width, MUCompatGLsizei height) { (void)mu::pipeline::GetLegacyRenderFacade().SetScissor({x,y,width,height}); }
void mu_glLineWidth(MUCompatGLfloat width) { (void)mu::pipeline::GetLegacyRenderFacade().SetLineWidth(width); }
void mu_glBindTexture(MUCompatGLenum, MUCompatGLuint texture) { s_boundTexture = texture; mu::pipeline::GetLegacyRenderFacade().BindTextureId(texture); }
void mu_glGenTextures(MUCompatGLsizei n, MUCompatGLuint* textures)
{
    if (n <= 0 || textures == nullptr)
    {
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        textures[i] = AllocateDynamicTextureId();
        if (textures[i] == 0u)
        {
            std::fill(textures + i, textures + n, 0u);
            return;
        }
    }
}
void mu_glDeleteTextures(MUCompatGLsizei n, const MUCompatGLuint* textures)
{
    if (!textures)
    {
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        mu::GetRenderer().ReleaseTexture(textures[i]);
    }
}
void mu_glTexImage2D(MUCompatGLenum, MUCompatGLint, MUCompatGLint, MUCompatGLsizei width, MUCompatGLsizei height, MUCompatGLint, MUCompatGLenum format, MUCompatGLenum type, const void* pixels)
{
    mu::GetRenderer().EnsureTexture(s_boundTexture, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    std::vector<std::uint8_t> rgbaScratch;
    const void* uploadPixels = PrepareTextureUpload(format, type, pixels, width, height, rgbaScratch);
    mu::GetRenderer().QueueTextureUpdate(s_boundTexture, uploadPixels, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
}
void mu_glTexSubImage2D(MUCompatGLenum, MUCompatGLint, MUCompatGLint, MUCompatGLint, MUCompatGLsizei width, MUCompatGLsizei height, MUCompatGLenum format, MUCompatGLenum type, const void* pixels)
{
    std::vector<std::uint8_t> rgbaScratch;
    const void* uploadPixels = PrepareTextureUpload(format, type, pixels, width, height, rgbaScratch);
    mu::GetRenderer().QueueTextureUpdate(s_boundTexture, uploadPixels, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
}
void mu_glTexParameteri(MUCompatGLenum target, MUCompatGLenum pname, MUCompatGLint param) { mu::GetRenderer().SetTexParameter(static_cast<int>(target), static_cast<int>(pname), param); }
void mu_glTexEnvi(MUCompatGLenum, MUCompatGLenum, MUCompatGLint param) { (void)mu::pipeline::GetLegacyRenderFacade().SetTextureEnvironment(static_cast<mu::pipeline::RenderTextureEnvironment>(param)); }
void mu_glTexEnvf(MUCompatGLenum, MUCompatGLenum, MUCompatGLfloat param) { (void)mu::pipeline::GetLegacyRenderFacade().SetTextureEnvironment(static_cast<mu::pipeline::RenderTextureEnvironment>(static_cast<unsigned int>(param))); }
void mu_glAlphaFunc(MUCompatGLenum func, MUCompatGLfloat ref) { (void)mu::pipeline::GetLegacyRenderFacade().SetAlphaFunc(static_cast<mu::pipeline::RenderCompareFunction>(func), ref); }
void mu_glStencilFunc(MUCompatGLenum func, MUCompatGLint ref, MUCompatGLuint mask) { (void)mu::pipeline::GetLegacyRenderFacade().SetStencilFunc(static_cast<mu::pipeline::RenderCompareFunction>(func), static_cast<unsigned int>(ref), mask); }
void mu_glStencilOp(MUCompatGLenum sfail, MUCompatGLenum dpfail, MUCompatGLenum dppass) { (void)mu::pipeline::GetLegacyRenderFacade().SetStencilOp(static_cast<mu::pipeline::RenderStencilOperation>(sfail), static_cast<mu::pipeline::RenderStencilOperation>(dpfail), static_cast<mu::pipeline::RenderStencilOperation>(dppass)); }
void mu_glColorMask(MUCompatGLboolean r, MUCompatGLboolean g, MUCompatGLboolean b, MUCompatGLboolean a) { (void)mu::pipeline::GetLegacyRenderFacade().SetColorMask(r != 0, g != 0, b != 0, a != 0); }
void mu_glPolygonMode(MUCompatGLenum face, MUCompatGLenum mode) { (void)mu::pipeline::GetLegacyRenderFacade().SetPolygonMode(static_cast<mu::pipeline::RenderCullFace>(face), static_cast<mu::pipeline::RenderPolygonMode>(mode)); }
void mu_glCullFace(MUCompatGLenum face) { (void)mu::pipeline::GetLegacyRenderFacade().SetCullFace(static_cast<mu::pipeline::RenderCullFace>(face)); }
void mu_glFrontFace(MUCompatGLenum mode) { (void)mu::pipeline::GetLegacyRenderFacade().SetFrontFace(static_cast<mu::pipeline::RenderFrontFace>(mode)); }
void mu_glFogf(MUCompatGLenum pname, MUCompatGLfloat param)
{
    auto& facade = mu::pipeline::GetLegacyRenderFacade();
    if (pname == kGLFogDensity) { s_fogParams.density = param; (void)facade.SetFogDensity(param); }
    else if (pname == kGLFogStart) { s_fogParams.start = param; (void)facade.SetFogRange(s_fogParams.start, s_fogParams.end); }
    else if (pname == kGLFogEnd) { s_fogParams.end = param; (void)facade.SetFogRange(s_fogParams.start, s_fogParams.end); }
}
void mu_glFogi(MUCompatGLenum pname, MUCompatGLint param)
{
    if (pname == kGLFogMode) { s_fogParams.mode = param; (void)mu::pipeline::GetLegacyRenderFacade().SetFogMode(static_cast<mu::pipeline::RenderFogMode>(param)); }
}
void mu_glFogfv(MUCompatGLenum pname, const MUCompatGLfloat* params)
{
    if (pname == kGLFogColor && params)
    {
        s_fogParams.color[0] = params[0];
        s_fogParams.color[1] = params[1];
        s_fogParams.color[2] = params[2];
        s_fogParams.color[3] = params[3];
        (void)mu::pipeline::GetLegacyRenderFacade().SetFogColor({params[0],params[1],params[2],params[3]});
    }
}
void mu_glShadeModel(MUCompatGLenum mode) { (void)mu::pipeline::GetLegacyRenderFacade().SetShadeMode(static_cast<mu::pipeline::RenderShadeMode>(mode)); }
void mu_glReadPixels(MUCompatGLint x, MUCompatGLint y, MUCompatGLsizei width, MUCompatGLsizei height, MUCompatGLenum, MUCompatGLenum, void* pixels) { mu::GetRenderer().ReadPixels(x, y, width, height, pixels); }
const MUCompatGLubyte* mu_glGetString(MUCompatGLenum)
{
    return reinterpret_cast<const MUCompatGLubyte*>(mu::GetRenderer().GetGPUDriverName());
}
void mu_gluPerspective(MUCompatGLdouble fovy, MUCompatGLdouble aspect, MUCompatGLdouble zNear, MUCompatGLdouble zFar)
{
    const glm::mat4 perspective = glm::perspective(glm::radians(static_cast<float>(fovy)),
                                                   static_cast<float>(aspect),
                                                   static_cast<float>(zNear),
                                                   static_cast<float>(zFar));
    mu::GetRenderer().MultMatrix(glm::value_ptr(perspective));
}
void mu_gluOrtho2D(MUCompatGLdouble left, MUCompatGLdouble right, MUCompatGLdouble bottom, MUCompatGLdouble top)
{
    const glm::mat4 ortho = glm::ortho(static_cast<float>(left), static_cast<float>(right),
                                       static_cast<float>(bottom), static_cast<float>(top));
    mu::GetRenderer().MultMatrix(glm::value_ptr(ortho));
}
void* mu_gluNewQuadric() { return nullptr; }
void mu_gluSphere(void*, MUCompatGLdouble, MUCompatGLint, MUCompatGLint) {}
void mu_glEnableClientState(MUCompatGLenum array)
{
    if (array == kGLVertexArray) s_vertexPointer.enabled = true;
    else if (array == kGLColorArray) s_colorPointer.enabled = true;
    else if (array == kGLTextureCoordArray) s_texCoordPointer.enabled = true;

    mu::pipeline::GetLegacyRenderFacade().EnableClientArray(
        static_cast<mu::pipeline::RenderClientArraySemantic>(array));
}

void mu_glDisableClientState(MUCompatGLenum array)
{
    if (array == kGLVertexArray) s_vertexPointer.enabled = false;
    else if (array == kGLColorArray) s_colorPointer.enabled = false;
    else if (array == kGLTextureCoordArray) s_texCoordPointer.enabled = false;

    mu::pipeline::GetLegacyRenderFacade().DisableClientArray(
        static_cast<mu::pipeline::RenderClientArraySemantic>(array));
}

void mu_glVertexPointer(MUCompatGLint size, MUCompatGLenum type, MUCompatGLsizei stride, const void* pointer)
{
    s_vertexPointer = {size, type, stride, pointer, s_vertexPointer.enabled};
}

void mu_glTexCoordPointer(MUCompatGLint size, MUCompatGLenum type, MUCompatGLsizei stride, const void* pointer)
{
    s_texCoordPointer = {size, type, stride, pointer, s_texCoordPointer.enabled};
}

void mu_glColorPointer(MUCompatGLint size, MUCompatGLenum type, MUCompatGLsizei stride, const void* pointer)
{
    s_colorPointer = {size, type, stride, pointer, s_colorPointer.enabled};
}

void mu_glDrawArrays(MUCompatGLenum mode, MUCompatGLint first, MUCompatGLsizei count)
{
    if (first < 0 || count < 0 || !s_vertexPointer.enabled || !s_vertexPointer.pointer)
        return;

    auto upload = [first, count](const ClientPointerState& state,
                                 mu::pipeline::RenderClientArraySemantic semantic,
                                 bool normalized) -> bool
    {
        if (!state.enabled || !state.pointer)
            return true;

        std::size_t scalar = 0;
        switch (state.type)
        {
        case 0x1400:
        case 0x1401: scalar = 1; break;
        case 0x1402:
        case 0x1403: scalar = 2; break;
        case 0x1404:
        case 0x1405:
        case kGLFloat: scalar = 4; break;
        case 0x140A: scalar = 8; break;
        default: return false;
        }

        if (state.size <= 0 || count == 0)
            return count == 0;

        const std::size_t elementBytes = scalar * static_cast<std::size_t>(state.size);
        const std::size_t strideBytes = state.stride > 0 ? static_cast<std::size_t>(state.stride) : elementBytes;
        const std::size_t last = static_cast<std::size_t>(first + count - 1);
        if (last > (static_cast<std::size_t>(-1) - elementBytes) / strideBytes)
            return false;

        const std::size_t bytesNeeded = last * strideBytes + elementBytes;
        const auto* bytes = static_cast<const std::byte*>(state.pointer);
        return mu::pipeline::GetLegacyRenderFacade().SetClientArray(
            semantic, std::span<const std::byte>(bytes, bytesNeeded),
            static_cast<std::uint32_t>(state.size),
            static_cast<mu::pipeline::RenderClientArrayScalarType>(state.type),
            state.stride, normalized);
    };

    if (!upload(s_vertexPointer, mu::pipeline::RenderClientArraySemantic::Vertex, false))
        return;
    if (!upload(s_colorPointer, mu::pipeline::RenderClientArraySemantic::Color, s_colorPointer.type != kGLFloat))
        return;
    if (!upload(s_texCoordPointer, mu::pipeline::RenderClientArraySemantic::TextureCoordinate, false))
        return;

    (void)mu::pipeline::GetLegacyRenderFacade().DrawArrays(
        static_cast<mu::pipeline::LegacyPrimitive>(mode), first, count);
}
void mu_glReadBuffer(MUCompatGLenum) {}
void mu_glFlush() {}
void mu_glPixelStorei(MUCompatGLenum, MUCompatGLint) {}
void mu_glPushAttrib(MUCompatGLbitfield) { (void)mu::pipeline::GetLegacyRenderFacade().PushAttrib(); }
void mu_glPopAttrib() { (void)mu::pipeline::GetLegacyRenderFacade().PopAttrib(); }
void mu_glPushClientAttrib(MUCompatGLbitfield) { (void)mu::pipeline::GetLegacyRenderFacade().PushClientAttrib(); }
void mu_glPopClientAttrib() { (void)mu::pipeline::GetLegacyRenderFacade().PopClientAttrib(); }
void mu_glNormal3f(MUCompatGLfloat x, MUCompatGLfloat y, MUCompatGLfloat z)
{
    s_nx = x;
    s_ny = y;
    s_nz = z;
    (void)mu::pipeline::GetLegacyRenderFacade().Normal3(x, y, z);
}
void mu_glNormal3fv(const MUCompatGLfloat* normal)
{
    if (normal) mu_glNormal3f(normal[0], normal[1], normal[2]);
}
void mu_glCopyTexImage2D(MUCompatGLenum, MUCompatGLint, MUCompatGLenum, MUCompatGLint, MUCompatGLint, MUCompatGLsizei, MUCompatGLsizei, MUCompatGLint) {}
