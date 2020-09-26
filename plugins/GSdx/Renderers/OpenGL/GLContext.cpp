#include "stdafx.h"
#include "Renderers/OpenGL/GLContext.h"

#if defined(_WIN32)
#include "GLContext/GLContextWGL.h"
#endif

std::unique_ptr<GLContext> GLContext::Create(void* handle)
{
	std::unique_ptr<GLContext> context;

	#if defined(_WIN32)
	context = std::make_unique<GLContextWGL>();
	#endif

	if (!context)
		return nullptr;

	if (!context->Init(handle))
		return nullptr;

	return context;
}