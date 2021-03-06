#pragma once

#include "OpenGLCommonHeaders.h"

namespace Havana::Graphics::OpenGL::Core
{
	bool Initialize();
	void Shutdown();
	void Render();

	Surface CreateSurface(Platform::Window window);
	void RemoveSurface(surface_id id);
	void ResizeSurface(surface_id id, u32, u32);
	u32 SurfaceWidth(surface_id id);
	u32 SurfaceHeight(surface_id id);
	void RenderSurface(surface_id id);
}