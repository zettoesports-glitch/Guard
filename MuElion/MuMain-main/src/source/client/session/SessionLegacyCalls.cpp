#include "stdafx.h"
#include "client/session/SessionLegacyCalls.h"

#include <array>
#include <cstring>
#include <span>

namespace mu::session
{

namespace
{
constexpr unsigned int GL_TEXTURE_2D_VALUE = 0x0DE1;
constexpr unsigned int GL_DEPTH_TEST_VALUE = 0x0B71;
constexpr unsigned int GL_BLEND_VALUE = 0x0BE2;
constexpr unsigned int GL_ALPHA_TEST_VALUE = 0x0BC0;
constexpr unsigned int GL_CULL_FACE_VALUE = 0x0B44;
constexpr unsigned int GL_FOG_VALUE = 0x0B60;
constexpr unsigned int GL_STENCIL_TEST_VALUE = 0x0B90;
constexpr unsigned int GL_SCISSOR_TEST_VALUE = 0x0C11;

constexpr unsigned int GL_COLOR_BUFFER_BIT_VALUE = 0x00004000;
constexpr unsigned int GL_DEPTH_BUFFER_BIT_VALUE = 0x00000100;
constexpr unsigned int GL_STENCIL_BUFFER_BIT_VALUE = 0x00000400;

constexpr unsigned int GL_FOG_MODE_VALUE = 0x0B65;
constexpr unsigned int GL_FOG_DENSITY_VALUE = 0x0B62;
constexpr unsigned int GL_FOG_START_VALUE = 0x0B63;
constexpr unsigned int GL_FOG_END_VALUE = 0x0B64;
constexpr unsigned int GL_FOG_COLOR_VALUE = 0x0B66;

mu::pipeline::RenderClientArraySemantic ArraySemantic(unsigned int array)
{
    return static_cast<mu::pipeline::RenderClientArraySemantic>(array);
}
} // namespace

#pragma push_macro("glBegin")
#pragma push_macro("glEnd")
#pragma push_macro("glVertex3fv")
#pragma push_macro("glColor3fv")
#pragma push_macro("glColor4fv")
#pragma push_macro("glNormal3fv")
#pragma push_macro("glTexCoord2fv")
#pragma push_macro("glEnable")
#pragma push_macro("glDisable")
#pragma push_macro("glDepthFunc")
#pragma push_macro("glBlendFunc")
#pragma push_macro("glAlphaFunc")
#pragma push_macro("glCullFace")
#pragma push_macro("glFrontFace")
#pragma push_macro("glPolygonMode")
#pragma push_macro("glMatrixMode")
#pragma push_macro("glLoadMatrixf")
#pragma push_macro("glMultMatrixf")
#pragma push_macro("glPushAttrib")
#pragma push_macro("glPushClientAttrib")
#pragma push_macro("glViewport")
#pragma push_macro("glScissor")
#pragma push_macro("glClear")
#pragma push_macro("glStencilFunc")
#pragma push_macro("glStencilOp")
#pragma push_macro("glTexEnvi")
#pragma push_macro("glTexEnvf")
#pragma push_macro("glFogf")
#pragma push_macro("glFogfv")
#pragma push_macro("glFogi")
#pragma push_macro("glEnableClientState")
#pragma push_macro("glDisableClientState")
#pragma push_macro("glDrawArrays")
#pragma push_macro("glVertexPointer")
#pragma push_macro("glColorPointer")
#pragma push_macro("glTexCoordPointer")

#undef glBegin
#undef glEnd
#undef glVertex3fv
#undef glColor3fv
#undef glColor4fv
#undef glNormal3fv
#undef glTexCoord2fv
#undef glEnable
#undef glDisable
#undef glDepthFunc
#undef glBlendFunc
#undef glAlphaFunc
#undef glCullFace
#undef glFrontFace
#undef glPolygonMode
#undef glMatrixMode
#undef glLoadMatrixf
#undef glMultMatrixf
#undef glPushAttrib
#undef glPushClientAttrib
#undef glViewport
#undef glScissor
#undef glClear
#undef glStencilFunc
#undef glStencilOp
#undef glTexEnvi
#undef glTexEnvf
#undef glFogf
#undef glFogfv
#undef glFogi
#undef glEnableClientState
#undef glDisableClientState
#undef glDrawArrays
#undef glVertexPointer
#undef glColorPointer
#undef glTexCoordPointer

void SessionLegacyCalls::glBegin(unsigned int mode) const { (void)m_facade.Begin(static_cast<mu::pipeline::LegacyPrimitive>(mode)); }
void SessionLegacyCalls::glEnd() const { (void)m_facade.End(); }
void SessionLegacyCalls::glVertex3fv(const float* v) const { if (v) (void)m_facade.Vertex3(v[0], v[1], v[2]); }
void SessionLegacyCalls::glColor3fv(const float* v) const { if (v) (void)m_facade.Color4(v[0], v[1], v[2], 1.0f); }
void SessionLegacyCalls::glColor4fv(const float* v) const { if (v) (void)m_facade.Color4(v[0], v[1], v[2], v[3]); }
void SessionLegacyCalls::glNormal3fv(const float* v) const { if (v) (void)m_facade.Normal3(v[0], v[1], v[2]); }
void SessionLegacyCalls::glTexCoord2fv(const float* v) const { if (v) (void)m_facade.TexCoord2(v[0], v[1]); }

void SessionLegacyCalls::glEnable(unsigned int cap) const
{
    if (cap == GL_TEXTURE_2D_VALUE) (void)m_facade.SetTextureEnable(true);
    else if (cap == GL_DEPTH_TEST_VALUE) (void)m_facade.SetDepthTestEnable(true);
    else if (cap == GL_BLEND_VALUE) (void)m_facade.SetBlendEnable(true);
    else if (cap == GL_ALPHA_TEST_VALUE) (void)m_facade.SetAlphaTestEnable(true);
    else if (cap == GL_CULL_FACE_VALUE) (void)m_facade.SetCullEnable(true);
    else if (cap == GL_FOG_VALUE) (void)m_facade.SetFogEnable(true);
    else if (cap == GL_STENCIL_TEST_VALUE) (void)m_facade.SetStencilEnable(true);
    else if (cap == GL_SCISSOR_TEST_VALUE) (void)m_facade.SetScissorEnable(true);
}

void SessionLegacyCalls::glDisable(unsigned int cap) const
{
    if (cap == GL_TEXTURE_2D_VALUE) (void)m_facade.SetTextureEnable(false);
    else if (cap == GL_DEPTH_TEST_VALUE) (void)m_facade.SetDepthTestEnable(false);
    else if (cap == GL_BLEND_VALUE) (void)m_facade.SetBlendEnable(false);
    else if (cap == GL_ALPHA_TEST_VALUE) (void)m_facade.SetAlphaTestEnable(false);
    else if (cap == GL_CULL_FACE_VALUE) (void)m_facade.SetCullEnable(false);
    else if (cap == GL_FOG_VALUE) (void)m_facade.SetFogEnable(false);
    else if (cap == GL_STENCIL_TEST_VALUE) (void)m_facade.SetStencilEnable(false);
    else if (cap == GL_SCISSOR_TEST_VALUE) (void)m_facade.SetScissorEnable(false);
}

void SessionLegacyCalls::glDepthFunc(unsigned int func) const { (void)m_facade.SetDepthFunc(static_cast<mu::pipeline::RenderCompareFunction>(func)); }
void SessionLegacyCalls::glBlendFunc(unsigned int s, unsigned int d) const { (void)m_facade.SetBlendFunc(static_cast<mu::pipeline::RenderBlendFactor>(s), static_cast<mu::pipeline::RenderBlendFactor>(d)); }
void SessionLegacyCalls::glAlphaFunc(unsigned int func, float ref) const { (void)m_facade.SetAlphaFunc(static_cast<mu::pipeline::RenderCompareFunction>(func), ref); }
void SessionLegacyCalls::glCullFace(unsigned int face) const { (void)m_facade.SetCullFace(static_cast<mu::pipeline::RenderCullFace>(face)); }
void SessionLegacyCalls::glFrontFace(unsigned int face) const { (void)m_facade.SetFrontFace(static_cast<mu::pipeline::RenderFrontFace>(face)); }
void SessionLegacyCalls::glPolygonMode(unsigned int face, unsigned int mode) const { (void)m_facade.SetPolygonMode(static_cast<mu::pipeline::RenderCullFace>(face), static_cast<mu::pipeline::RenderPolygonMode>(mode)); }
void SessionLegacyCalls::glMatrixMode(unsigned int mode) const { (void)m_facade.MatrixMode(static_cast<mu::pipeline::LegacyMatrixMode>(mode)); }

void SessionLegacyCalls::glLoadMatrixf(const float* matrix) const
{
    if (!matrix) return;
    std::array<float, 16> copy{};
    std::memcpy(copy.data(), matrix, sizeof(copy));
    (void)m_facade.LoadMatrix(copy);
}

void SessionLegacyCalls::glMultMatrixf(const float* matrix) const
{
    if (!matrix) return;
    std::array<float, 16> copy{};
    std::memcpy(copy.data(), matrix, sizeof(copy));
    (void)m_facade.MultMatrix(copy);
}

void SessionLegacyCalls::glPushAttrib(unsigned int) const {}
void SessionLegacyCalls::glPushClientAttrib(unsigned int) const {}
void SessionLegacyCalls::glViewport(int x, int y, int w, int h) const { (void)m_facade.SetViewport({x, y, w, h}); }
void SessionLegacyCalls::glScissor(int x, int y, int w, int h) const { (void)m_facade.SetScissor({x, y, w, h}); }

void SessionLegacyCalls::glClear(unsigned int mask) const
{
    (void)m_facade.Clear((mask & GL_COLOR_BUFFER_BIT_VALUE) != 0,
                         (mask & GL_DEPTH_BUFFER_BIT_VALUE) != 0,
                         (mask & GL_STENCIL_BUFFER_BIT_VALUE) != 0);
}

void SessionLegacyCalls::glStencilFunc(unsigned int func, int ref, unsigned int mask) const { (void)m_facade.SetStencilFunc(static_cast<mu::pipeline::RenderCompareFunction>(func), static_cast<unsigned int>(ref), mask); }
void SessionLegacyCalls::glStencilOp(unsigned int a, unsigned int b, unsigned int c) const { (void)m_facade.SetStencilOp(static_cast<mu::pipeline::RenderStencilOperation>(a), static_cast<mu::pipeline::RenderStencilOperation>(b), static_cast<mu::pipeline::RenderStencilOperation>(c)); }
void SessionLegacyCalls::glTexEnvi(unsigned int, unsigned int, int param) const { (void)m_facade.SetTextureEnvironment(static_cast<mu::pipeline::RenderTextureEnvironment>(param)); }
void SessionLegacyCalls::glTexEnvf(unsigned int, unsigned int, float param) const { (void)m_facade.SetTextureEnvironment(static_cast<mu::pipeline::RenderTextureEnvironment>(static_cast<unsigned int>(param))); }

void SessionLegacyCalls::glFogf(unsigned int pname, float param) const
{
    if (pname == GL_FOG_DENSITY_VALUE) (void)m_facade.SetFogDensity(param);
    else if (pname == GL_FOG_START_VALUE) (void)m_facade.SetFogRange(param, 1.0f);
    else if (pname == GL_FOG_END_VALUE) (void)m_facade.SetFogRange(0.0f, param);
}

void SessionLegacyCalls::glFogfv(unsigned int pname, const float* params) const
{
    if (pname == GL_FOG_COLOR_VALUE && params)
        (void)m_facade.SetFogColor({params[0], params[1], params[2], params[3]});
}

void SessionLegacyCalls::glFogi(unsigned int pname, int param) const
{
    if (pname == GL_FOG_MODE_VALUE)
        (void)m_facade.SetFogMode(static_cast<mu::pipeline::RenderFogMode>(param));
}

void SessionLegacyCalls::glShadeModel(unsigned int mode) const
{
    (void)m_facade.SetShadeMode(
        static_cast<mu::pipeline::RenderShadeMode>(mode));
}

void SessionLegacyCalls::glEnableClientState(unsigned int array) const { m_facade.EnableClientArray(ArraySemantic(array)); }
void SessionLegacyCalls::glDisableClientState(unsigned int array) const { m_facade.DisableClientArray(ArraySemantic(array)); }

void SessionLegacyCalls::glDrawArrays(unsigned int mode, int first, int count) const
{
    if (first < 0 || count < 0)
        return;

    if (m_vertexPointer.pointer &&
        !UploadPointer(m_vertexPointer, mu::pipeline::RenderClientArraySemantic::Vertex, first, count))
        return;
    if (m_colorPointer.pointer)
        (void)UploadPointer(m_colorPointer, mu::pipeline::RenderClientArraySemantic::Color, first, count);
    if (m_texCoordPointer.pointer)
        (void)UploadPointer(m_texCoordPointer, mu::pipeline::RenderClientArraySemantic::TextureCoordinate, first, count);

    (void)m_facade.DrawArrays(static_cast<mu::pipeline::LegacyPrimitive>(mode), first, count);
}

std::size_t SessionLegacyCalls::ScalarSize(unsigned int type) noexcept
{
    switch (static_cast<mu::pipeline::RenderClientArrayScalarType>(type))
    {
    case mu::pipeline::RenderClientArrayScalarType::Byte:
    case mu::pipeline::RenderClientArrayScalarType::UnsignedByte: return 1;
    case mu::pipeline::RenderClientArrayScalarType::Short:
    case mu::pipeline::RenderClientArrayScalarType::UnsignedShort: return 2;
    case mu::pipeline::RenderClientArrayScalarType::Int:
    case mu::pipeline::RenderClientArrayScalarType::UnsignedInt:
    case mu::pipeline::RenderClientArrayScalarType::Float: return 4;
    case mu::pipeline::RenderClientArrayScalarType::Double: return 8;
    default: return 0;
    }
}

void SessionLegacyCalls::SetPointer(PointerDescriptor& descriptor, int size, unsigned int type,
                                    int stride, const void* pointer, bool normalized) const noexcept
{
    if (!pointer || size <= 0 || ScalarSize(type) == 0)
    {
        descriptor = {};
        return;
    }

    descriptor.size = size;
    descriptor.type = type;
    descriptor.stride = stride;
    descriptor.pointer = pointer;
    descriptor.normalized = normalized;
}

bool SessionLegacyCalls::UploadPointer(const PointerDescriptor& descriptor,
                                       mu::pipeline::RenderClientArraySemantic semantic,
                                       int first, int count) const noexcept
{
    if (!descriptor.pointer || descriptor.size <= 0 || first < 0 || count < 0)
        return false;
    if (count == 0)
        return true;

    const std::size_t scalar = ScalarSize(descriptor.type);
    const std::size_t elementBytes = scalar * static_cast<std::size_t>(descriptor.size);
    const std::size_t stride = descriptor.stride > 0 ? static_cast<std::size_t>(descriptor.stride) : elementBytes;
    const std::size_t last = static_cast<std::size_t>(first + count - 1);

    if (last > (static_cast<std::size_t>(-1) - elementBytes) / stride)
        return false;

    const std::size_t totalBytes = last * stride + elementBytes;
    const auto* bytes = static_cast<const std::byte*>(descriptor.pointer);
    return m_facade.SetClientArray(semantic, std::span<const std::byte>(bytes, totalBytes),
                                   static_cast<std::uint32_t>(descriptor.size),
                                   static_cast<mu::pipeline::RenderClientArrayScalarType>(descriptor.type),
                                   descriptor.stride, descriptor.normalized);
}

void SessionLegacyCalls::glVertexPointer(int size, unsigned int type, int stride, const void* pointer) const { SetPointer(m_vertexPointer, size, type, stride, pointer, false); }
void SessionLegacyCalls::glColorPointer(int size, unsigned int type, int stride, const void* pointer) const { SetPointer(m_colorPointer, size, type, stride, pointer, type != static_cast<unsigned int>(mu::pipeline::RenderClientArrayScalarType::Float)); }
void SessionLegacyCalls::glTexCoordPointer(int size, unsigned int type, int stride, const void* pointer) const { SetPointer(m_texCoordPointer, size, type, stride, pointer, false); }

#pragma pop_macro("glTexCoordPointer")
#pragma pop_macro("glColorPointer")
#pragma pop_macro("glVertexPointer")
#pragma pop_macro("glDrawArrays")
#pragma pop_macro("glDisableClientState")
#pragma pop_macro("glEnableClientState")
#pragma pop_macro("glFogi")
#pragma pop_macro("glFogfv")
#pragma pop_macro("glFogf")
#pragma pop_macro("glTexEnvf")
#pragma pop_macro("glTexEnvi")
#pragma pop_macro("glStencilOp")
#pragma pop_macro("glStencilFunc")
#pragma pop_macro("glClear")
#pragma pop_macro("glScissor")
#pragma pop_macro("glViewport")
#pragma pop_macro("glPushClientAttrib")
#pragma pop_macro("glPushAttrib")
#pragma pop_macro("glMultMatrixf")
#pragma pop_macro("glLoadMatrixf")
#pragma pop_macro("glMatrixMode")
#pragma pop_macro("glPolygonMode")
#pragma pop_macro("glFrontFace")
#pragma pop_macro("glCullFace")
#pragma pop_macro("glAlphaFunc")
#pragma pop_macro("glBlendFunc")
#pragma pop_macro("glDepthFunc")
#pragma pop_macro("glDisable")
#pragma pop_macro("glEnable")
#pragma pop_macro("glTexCoord2fv")
#pragma pop_macro("glNormal3fv")
#pragma pop_macro("glColor4fv")
#pragma pop_macro("glColor3fv")
#pragma pop_macro("glVertex3fv")
#pragma pop_macro("glEnd")
#pragma pop_macro("glBegin")

SessionLegacyCalls& GetSessionLegacyCalls()
{
    static SessionLegacyCalls calls(mu::pipeline::GetLegacyRenderFacade());
    return calls;
}

} // namespace mu::session
