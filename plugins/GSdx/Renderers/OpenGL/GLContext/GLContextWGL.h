#pragma once

#include "Renderers/OpenGL/GLContext.h"

class GLContextWGL : public GLContext
{
	public:
		~GLContextWGL();
		bool MakeCurrent() override;
		bool FreeCurrent() override;

		bool SetSwapInterval(int interval) override;
		bool Swap() override;
		bool Init(void* handle) override;
	private:
		HGLRC CreateCoreContext(HDC device_context);

		HWND m_native_handle;
		HDC m_device_context;
		HGLRC m_rendering_context;
};