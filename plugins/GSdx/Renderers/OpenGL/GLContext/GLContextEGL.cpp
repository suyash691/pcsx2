#include "stdafx.h"

#include <array>
#include "Renderers/OpenGL/GLContext/GLContextEGL.h"

GLContextEGL::~GLContextEGL()
{
    if (m_context)
    {
        if(eglGetCurrentContext() == m_context)
        {
            eglMakeCurrent(
                m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
            );
        }
    }

    if(m_surface && eglGetCurrentSurface(EGL_DRAW) == m_surface)
        eglDestroySurface(m_display, m_surface);
}

bool GLContextEGL::Init(void** dsp)
{
    void* display = *dsp;
	void* window = (void*)((uptr*)(dsp)+1);

    if(!display || !window)
        return false;

    auto native_handle = reinterpret_cast<EGLNativeDisplayType>(display);

    m_display = eglGetDisplay(native_handle);
    if (!m_display)
    {
        fprintf(stderr, "GLContext: Failed to open display %d\n", eglGetError());
        return false;
    }

    EGLint major;
    EGLint minor;
    if (!eglInitialize(m_display, &major, &minor))
    {
        fprintf(stderr, "GLContext: Failed to init display %d\n", eglGetError());
        return false;
    }

    std::array<EGLint, 11> attribs =
    {
        EGL_RENDERABLE_TYPE, 0,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint config_count;
    if (!eglChooseConfig(m_display, attribs.data(), &config, 1, &config_count))
    {
        fprintf(stderr, "GLContext: Failed to get a visual config %d\n", eglGetError());
        return false;
    }

    // TODO: GLES?
    if (!eglBindAPI(EGL_OPENGL_API))
    {
        fprintf(stderr, "CLContext: Failed to bind API %d\n", eglGetError());
        return false;
    }

    // die early on this to help debugging
    std::string ext_list(eglQueryString(m_display, EGL_EXTENSIONS));
    if(ext_list.find("EGL_KHR_create_context") == std::string::npos)
    {
        fprintf(stderr, "GLContext: No support for core profile\n");
        return false;
    }

    constexpr std::array<std::pair<int, int>, 8> gl_versions =
	{
		{
			{4, 6}, {4, 5}, {4, 4}, {4, 3},
			{4, 2}, {4, 1}, {4, 0}, {3, 3}
		}
	};

    // major, minor
	// we need 3.3 at the very least but it doesn't
	// hurt to have something newer
    for (const auto& version : gl_versions)
    {
        std::array<EGLint, 9> core_attribs =
        {
            EGL_CONTEXT_MAJOR_VERSION_KHR,
            version.first,
            EGL_CONTEXT_MINOR_VERSION_KHR,
            version.second,
            EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
            EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
            EGL_CONTEXT_FLAGS_KHR,
            EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR,
            EGL_NONE
        };

        m_context = eglCreateContext(
            m_display, config, EGL_NO_CONTEXT, core_attribs.data()
        );

        if (m_context)
        {
            fprintf(stderr, "GLContext: Created context (%d, %d)\n",
                    version.first, version.second);
            break;
        }
    }

    if (!m_context)
    {
        fprintf(stderr, "GLContext: Failed to create a context\n");
        return false;
    }

    m_surface = eglCreatePlatformWindowSurface(
        m_display, config, window, nullptr
    );

    if (!m_surface)
    {
        fprintf(stderr, "GLContext: Failed to create surface %d\n", eglGetError());
        return false;
    }

    EGLint width;
    if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width))
    {
        fprintf(stderr, "GLContext: Failed to query surface width %d\n", eglGetError());
        return false;
    }

    EGLint height;
    if (!eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height))
    {
        fprintf(stderr, "GLContext: Failed to query surface height %d\n", eglGetError());
        return false;
    }

    m_width = width;
    m_height = height;

    return true;
}

bool GLContextEGL::MakeCurrent()
{
    const EGLBoolean res = eglMakeCurrent(
        m_display, m_surface, m_surface, m_context
    );

    return res == EGL_TRUE;
}

bool GLContextEGL::FreeCurrent()
{
    const EGLBoolean res = eglMakeCurrent(
        m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
    );

    return res == EGL_TRUE;
}

bool GLContextEGL::Swap()
{
    const EGLBoolean res = eglSwapBuffers(
        m_display, m_surface
    );

    return res == EGL_TRUE;
}

bool GLContextEGL::SetSwapInterval(int interval)
{
    const EGLBoolean res = eglSwapInterval(
        m_display, interval
    );

    return res == EGL_TRUE;
}