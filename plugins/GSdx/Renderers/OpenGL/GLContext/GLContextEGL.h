#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "Renderers/OpenGL/GLContext.h"

class GLContextEGL : public GLContext
{
	public:
		~GLContextEGL();
		bool MakeCurrent() override;
		bool FreeCurrent() override;

		bool SetSwapInterval(int interval) override;
		bool Swap() override;
		bool Init(void** dsp) override;
    protected:
		EGLSurface m_surface = EGL_NO_SURFACE;
		EGLContext m_context = EGL_NO_CONTEXT;
		EGLDisplay m_display = EGL_NO_DISPLAY;
};
