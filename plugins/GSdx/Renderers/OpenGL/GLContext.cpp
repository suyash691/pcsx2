#include "stdafx.h"
#include "Renderers/OpenGL/GLContext.h"

#if defined(_WIN32)
#include "GLContext/GLContextWGL.h"
#endif
#if defined(__unix__)
#include "Renderers/OpenGL/GLContext/GLContextEGL.h"
#endif

std::unique_ptr<GLContext> GLContext::Create(void** dsp)
{
	std::unique_ptr<GLContext> context;

	#if defined(_WIN32)
	context = std::make_unique<GLContextWGL>();
	#endif
	#if defined(__unix__)
	context = std::make_unique<GLContextEGL>();
	#endif

	if (!context)
		return nullptr;

	if (!context->Init(dsp))
		return nullptr;

	return context;
}