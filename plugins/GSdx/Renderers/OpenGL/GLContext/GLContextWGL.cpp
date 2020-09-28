#include "stdafx.h"

#include <array>
#include "Renderers/OpenGL/GLContext/GLContextWGL.h"

namespace
{
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

	void LoadExtensions()
	{
		wglSwapIntervalEXT =
			reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
				wglGetProcAddress("wglSwapIntervalEXT")
				);

		wglCreateContextAttribsARB =
			reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
				wglGetProcAddress("wglCreateContextAttribsARB")
				);
	}

	void UnloadExtensions()
	{
		wglSwapIntervalEXT = nullptr;
		wglCreateContextAttribsARB = nullptr;
	}
}

GLContextWGL::~GLContextWGL()
{
	if (m_rendering_context)
	{
		// we need to check that we are
		// releasing on the same thread
		if (wglGetCurrentContext() == m_rendering_context)
		{
			// if these functions fail it's not fatal
			// but it is cause for concern
			if (!wglMakeCurrent(m_device_context, nullptr))
				fprintf(stderr, "GLContext: Failed to release rendering context\n");

			if (!wglDeleteContext(m_rendering_context))
				fprintf(stderr, "GLContext: Failed to delete rendering context\n");

			m_rendering_context = nullptr;
		}
	}

	if (m_device_context)
	{
		if (!ReleaseDC(m_native_handle, m_device_context))
			fprintf(stderr, "GLContext: Failed to release drawing context\n");
	}
}

bool GLContextWGL::Init(void** dsp)
{
	void* handle = *dsp;

	if (!handle)
		return false;

	m_native_handle = reinterpret_cast<HWND>(handle);

	RECT rectangle = {};
	if (!GetClientRect(m_native_handle, &rectangle))
	{
		fprintf(stderr, "GLContext: Failed to get window rectangle\n");
		return false;
	}

	m_width = rectangle.right - rectangle.left;
	m_height = rectangle.bottom - rectangle.top;

	m_device_context = GetDC(m_native_handle);
	if (!m_device_context)
	{
		fprintf(stderr, "GLContext: Failed to get device context\n");
		return false;
	}

	// We need a pixel format for the display buffer
	// I took the pixel format desc from the original GSWinWGL code
	// TODO: Stereo3D maybe
	{
		// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor
		constexpr PIXELFORMATDESCRIPTOR pixel_format_desc =
		{
			sizeof(PIXELFORMATDESCRIPTOR),			// Size Of This Pixel Format Descriptor
			1,										// Version Number
			PFD_DRAW_TO_WINDOW |					// Format Must Support Window
			PFD_SUPPORT_OPENGL |					// Format Must Support OpenGL
			PFD_DOUBLEBUFFER,						// Must Support Double Buffering
			PFD_TYPE_RGBA,							// Request An RGBA Format
			32,										// Select Our Color Depth
			0, 0, 0, 0, 0, 0,						// Color Bits Ignored
			0,										// 8bit Alpha Buffer
			0,										// Shift Bit Ignored
			0,										// No Accumulation Buffer
			0, 0, 0, 0,								// Accumulation Bits Ignored
			0,										// 24Bit Z-Buffer (Depth Buffer)
			8,										// 8bit Stencil Buffer
			0,										// No Auxiliary Buffer
			PFD_MAIN_PLANE,							// Main Drawing Layer
			0,										// Reserved
			0, 0, 0									// Layer Masks Ignored
		};

		const int pixel_format = ChoosePixelFormat(
			m_device_context, &pixel_format_desc
		);

		if (!pixel_format)
		{
			fprintf(stderr, "GLContext: Failed to find a suitable pixel format\n");
			return false;
		}

		const BOOL res = SetPixelFormat(
			m_device_context, pixel_format, &pixel_format_desc
		);

		if (!res)
		{
			fprintf(stderr, "GLContext: Failed to set pixel format\n");
			return false;
		}
	}

	// We need a 2.0 context first.
	HGLRC temp_context = wglCreateContext(m_device_context);
	if (!temp_context)
	{
		fprintf(stderr, "GLContext: Failed to create temp context\n");
		return false;
	}

	// The context must be current to load the extra WGL extensions
	if (!wglMakeCurrent(m_device_context, temp_context))
	{
		fprintf(stderr, "GLContext: Failed to make temp context current\n");
		return false;
	}

	LoadExtensions();

	// this requires a little extra work
	m_rendering_context = CreateCoreContext(m_device_context);

	// delete the temp context even if
	// we failed to get a core context
	// Note: if it fails it's likely a bad sign thus the log
	if (!wglDeleteContext(temp_context))
		fprintf(stderr, "GLContext: Failed to delete temp context\n");

	if (!m_rendering_context)
	{
		fprintf(stderr, "GLContext: Failed to create context\n");
		return false;
	}

	return true;
}

HGLRC GLContextWGL::CreateCoreContext(HDC device_context)
{
	if (!wglCreateContextAttribsARB)
	{
		fprintf(stderr, "GLContext: wglCreateContextAttribsARB not loaded\n");
		return nullptr;
	}

	// major, minor
	// we need 3.3 at the very least but it doesn't
	// hurt to have something newer
	constexpr std::array<std::pair<int, int>, 8> gl_versions =
	{
		{
			{4, 6}, {4, 5}, {4, 4}, {4, 3},
			{4, 2}, {4, 1}, {4, 0}, {3, 3}
		}
	};

	for (const auto& version_pair : gl_versions)
	{
		std::array<int, 10> attribs =
		{
			WGL_CONTEXT_PROFILE_MASK_ARB,
			WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			#if defined(_ENABLE_OGL_DEBUG)
			WGL_CONTEXT_FLAGS_ARB,
			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
			#else
			WGL_CONTEXT_FLAGS_ARB,
			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			#endif
			WGL_CONTEXT_MAJOR_VERSION_ARB, version_pair.first,
			WGL_CONTEXT_MINOR_VERSION_ARB, version_pair.second,
		};

		// https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
		HGLRC context = wglCreateContextAttribsARB(
			device_context, nullptr, attribs.data()
		);

		if (context)
		{
			fprintf(stderr, "GLContext: Created context (%d, %d)\n",
					version_pair.first, version_pair.second);

			return context;
		}
	}

	fprintf(stderr, "GLContext: Failed to create a context\n");

	return nullptr;
}

bool GLContextWGL::MakeCurrent()
{
	const BOOL res = wglMakeCurrent(
		m_device_context, m_rendering_context
	);

	return res == TRUE;
}

bool GLContextWGL::FreeCurrent()
{
	const BOOL res = wglMakeCurrent(
		m_device_context, nullptr
	);

	return res == TRUE;
}

bool GLContextWGL::SetSwapInterval(int interval)
{
	if (wglSwapIntervalEXT && wglSwapIntervalEXT(interval) == TRUE)
		return true;

	fprintf(stderr, "GLContext: Failed to set swap interval\n");

	return false;
}

bool GLContextWGL::Swap()
{
	const BOOL res = SwapBuffers(
		m_device_context
	);

	return res == TRUE;
}