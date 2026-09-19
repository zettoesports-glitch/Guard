#pragma once

#include "client/render/LegacyRenderFacade.h"

#include <cstddef>
#include <cstdint>

namespace mu::session
{

class SessionLegacyCalls
{
public:
    explicit SessionLegacyCalls(mu::pipeline::LegacyRenderFacade& facade) noexcept : m_facade(facade) {}

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

    void glBegin(unsigned int mode) const;
    void glEnd() const;
    void glVertex3fv(const float* value) const;
    void glColor3fv(const float* value) const;
    void glColor4fv(const float* value) const;
    void glNormal3fv(const float* value) const;
    void glTexCoord2fv(const float* value) const;
    void glEnable(unsigned int cap) const;
    void glDisable(unsigned int cap) const;
    void glDepthFunc(unsigned int func) const;
    void glBlendFunc(unsigned int source, unsigned int destination) const;
    void glAlphaFunc(unsigned int func, float ref) const;
    void glCullFace(unsigned int face) const;
    void glFrontFace(unsigned int face) const;
    void glPolygonMode(unsigned int face, unsigned int mode) const;
    void glMatrixMode(unsigned int mode) const;
    void glLoadMatrixf(const float* matrix) const;
    void glMultMatrixf(const float* matrix) const;
    void glPushAttrib(unsigned int mask) const;
    void glPushClientAttrib(unsigned int mask) const;
    void glViewport(int x, int y, int width, int height) const;
    void glScissor(int x, int y, int width, int height) const;
    void glClear(unsigned int mask) const;
    void glStencilFunc(unsigned int func, int ref, unsigned int mask) const;
    void glStencilOp(unsigned int fail, unsigned int depthFail, unsigned int pass) const;
    void glTexEnvi(unsigned int target, unsigned int pname, int param) const;
    void glTexEnvf(unsigned int target, unsigned int pname, float param) const;
    void glFogf(unsigned int pname, float param) const;
    void glFogfv(unsigned int pname, const float* params) const;
    void glFogi(unsigned int pname, int param) const;
    void glEnableClientState(unsigned int array) const;
    void glDisableClientState(unsigned int array) const;
    void glDrawArrays(unsigned int mode, int first, int count) const;
    void glVertexPointer(int size, unsigned int type, int stride, const void* pointer) const;
    void glColorPointer(int size, unsigned int type, int stride, const void* pointer) const;
    void glTexCoordPointer(int size, unsigned int type, int stride, const void* pointer) const;

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

private:
    [[nodiscard]] static std::size_t ScalarSize(unsigned int type) noexcept;
    void SetPointer(mu::pipeline::RenderClientArraySemantic semantic, int size, unsigned int type,
                    int stride, const void* pointer, bool normalized) const;

    mu::pipeline::LegacyRenderFacade& m_facade;
};

SessionLegacyCalls& GetSessionLegacyCalls();

} // namespace mu::session
