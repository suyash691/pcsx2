#pragma once

class GLContext
{
	public:
		virtual bool MakeCurrent() = 0;
		virtual bool FreeCurrent() = 0;
		virtual bool SetSwapInterval(int interval) = 0;
		virtual bool Swap() = 0;

		// Factory to create a new GL context
		// based on the current OS/Windowing system
		// returns nullptr if it fails
		static std::unique_ptr<GLContext> Create(void** dsp);
	protected:
		virtual bool Init(void**) = 0;

		uint32 m_width;
		uint32 m_height;
};