#include "Platform.h"
#include "PlatformTypes.h"

namespace Havana::Platform
{
#ifdef _WIN64	// open window for DirectX context
	namespace
	{
		// Windows OS specific window info
		struct WindowInfo
		{
			HWND	hwnd{ nullptr };
			RECT	clientArea{ 0, 0, 1580, 950 };
			RECT	fullScreenArea{};
			POINT	topLeft{ 0,0 };
			DWORD	style{ WS_VISIBLE };
			bool	isFullscreen{ false };
			bool	isClosed{ false };
		};

		Utils::vector<WindowInfo> windows;

		/////////////////////////////////////////////////////////////////
		// TODO: this part will be handled by a free-list container later
		Utils::vector<u32> availableSlots;

		u32 AddToWindows(WindowInfo info)
		{
			u32 id{ U32_INVALID_ID };
			if (availableSlots.empty())
			{
				id = (u32)windows.size();
				windows.emplace_back(info);
			}
			else
			{
				id = availableSlots.back();
				availableSlots.pop_back();
				assert(id != U32_INVALID_ID);
				windows[id] = info;
			}

			return id;
		}

		void RemoveFromWindows(u32 id)
		{
			assert(id < windows.size());
			availableSlots.emplace_back(id);
		}
		/////////////////////////////////////////////////////////////////

		WindowInfo& GetFromId(window_id id)
		{
			assert(id < windows.size());
			assert(windows[id].hwnd);
			return windows[id];
		}

		WindowInfo& GetFromHandle(window_handle handle)
		{
			const window_id id{ (Id::id_type)GetWindowLongPtr(handle, GWLP_USERDATA) };
			return GetFromId(id);
		}
		
		// Callback method for message handling
		LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
		{
			WindowInfo* info{ nullptr };

			switch (msg)
			{
			case WM_DESTROY:
				GetFromHandle(hwnd).isClosed = true;
				break;
			case WM_EXITSIZEMOVE:
				info = &GetFromHandle(hwnd);
				break;
			case WM_SIZE:
				if (wparam == SIZE_MAXIMIZED)
				{
					info = &GetFromHandle(hwnd);
				}
				break;
			case WM_SYSCOMMAND:
				if (wparam == SC_RESTORE)
				{
					info = &GetFromHandle(hwnd);
				}
				break;
			default:
				break;
			}

			if (info) // if not null, something changed that needs handling
			{
				assert(info->hwnd);
				GetClientRect(info->hwnd, info->isFullscreen ? &info->fullScreenArea : &info->clientArea);
			}
			
			// "Extra" bytes for handling windows messages via callback
			LONG_PTR longPtr{ GetWindowLongPtr(hwnd, 0) };
			return longPtr
				? ((window_proc)longPtr)(hwnd, msg, wparam, lparam)
				: DefWindowProc(hwnd, msg, wparam, lparam);
		}

		// Windows specific Window class function implementations
		void ResizeWindow(const WindowInfo& info, const RECT& area)
		{
			// Adjust the window size for correct device size
			RECT windowRect{ area };
			AdjustWindowRect(&windowRect, info.style, FALSE);

			const s32 width{ windowRect.right - windowRect.left };
			const s32 height{ windowRect.bottom - windowRect.top };

			MoveWindow(info.hwnd, info.topLeft.x, info.topLeft.y, width, height, true);
		}

		void ResizeWindow(window_id id, u32 width, u32 height)
		{
			WindowInfo& info{ GetFromId(id) };

			// NOTE: when we host a window in the level editor we just update
			// the internal data (i.e. the client area dimensions).
			if (info.style & WS_CHILD)
			{
				GetClientRect(info.hwnd, &info.clientArea);
			}
			else
			{
				// NOTE: resize in fullscreen mode as well to support the case when 
				// the user changes screen resolution
				RECT& area{ info.isFullscreen ? info.fullScreenArea : info.clientArea };
				area.bottom = area.top + height;
				area.right = area.left + width;

				ResizeWindow(info, area);
			}
		}

		void SetWindowFullscreen(window_id id, bool isFullscreen)
		{
			WindowInfo& info{ GetFromId(id) };
			if (info.isFullscreen != isFullscreen)
			{
				info.isFullscreen = isFullscreen;

				if (isFullscreen)
				{
					// Store the current window position and dimenstions so it can be restored
					// when switching out of a full screen state.
					GetClientRect(info.hwnd, &info.clientArea);
					RECT rect;
					GetWindowRect(info.hwnd, &rect);
					info.topLeft.x = rect.left;
					info.topLeft.y = rect.top;
					SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
					ShowWindow(info.hwnd, SW_MAXIMIZE);
				}
				else
				{
					SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
					ResizeWindow(info, info.clientArea);
					ShowWindow(info.hwnd, SW_SHOWNORMAL);
				}
			}
		}

		bool IsWindowFullscreen(window_id id)
		{
			return GetFromId(id).isFullscreen;
		}

		window_handle GetWindowHandle(window_id id)
		{
			return GetFromId(id).hwnd;
		}

		void SetWindowCaption(window_id id, const wchar_t* caption)
		{
			WindowInfo& info{ GetFromId(id) };
			SetWindowText(info.hwnd, caption);
		}

		Math::Vec4u32 GetWindowSize(window_id id)
		{
			WindowInfo& info{ GetFromId(id) };
			RECT& area{ info.isFullscreen ? info.fullScreenArea : info.clientArea };
			return { (u32)area.left, (u32)area.top, (u32)area.right, (u32)area.bottom };
		}

		bool IsWindowClosed(window_id id)
		{
			return GetFromId(id).isClosed;
		}
	} // anonymous namespace

	/// <summary>
	/// Create a new window.
	/// </summary>
	/// <param name="initInfo"> - Initialization information for the new window.</param>
	/// <returns>A Havana::Platform::Window object.</returns>
	Window MakeWindow(const WindowInitInfo* const initInfo /*= nullptr*/) // NOTEL CreateWindow collides with Windows.h
	{
		window_proc callback{ initInfo ? initInfo->callback : nullptr };
		window_handle parent{ initInfo ? initInfo->parent : nullptr };

		// Setup a window class
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = internal_window_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = callback ? sizeof(callback) : 0;
		wc.hInstance = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"HavanaWindow";
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		// Register the window class
		RegisterClassEx(&wc);

		// Create an instance of WindowInfo
		WindowInfo info{};
		info.clientArea.right = (initInfo && initInfo->width) ? info.clientArea.left + initInfo->width : info.clientArea.right;
		info.clientArea.bottom = (initInfo && initInfo->height) ? info.clientArea.top + initInfo->height : info.clientArea.bottom;
		info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;
		RECT rect{ info.clientArea };
		
		// Adjust the window size for correct device size
		AdjustWindowRect(&rect, info.style, FALSE);

		// check for initial info, use defaults if none given
		const wchar_t* caption{ (initInfo && initInfo->caption) ? initInfo->caption : L"Havana Game" };
		const s32 left{ initInfo ? initInfo->left : info.topLeft.x };
		const s32 top{ initInfo ? initInfo->top : info.topLeft.y };
		const s32 width{ rect.right - rect.left };
		const s32 height{ rect.bottom - rect.top };

		// Create instance of the window class
		info.hwnd = CreateWindowEx(
			0,					// extended style
			wc.lpszClassName,	// window class name
			caption,			// instance title
			info.style,			// window style
			left, top,			// initial window coords
			width, height,		// initial window dimensions
			parent,				// handle to parent window
			NULL,				// handle to menu
			NULL,				// instance of this application
			NULL);				// extra creation parameters

		if (info.hwnd)
		{
			DEBUG_OP(SetLastError(0));
			const window_id id{ AddToWindows(info) };
			// Include the window ID in the window class data structure
			SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);

			// Set  the pointer to the window callback function which handles messages
			// for the windows in the "extra" bytes
			if (callback) SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
			assert(GetLastError() == 0);

			ShowWindow(info.hwnd, SW_SHOWNORMAL);
			UpdateWindow(info.hwnd);
			return Window{ id };
		}
		
		return {};
	}

	/// <summary>
	/// Remove an existing window.
	/// </summary>
	/// <param name="id"> - ID number of the window to remove.</param>
	void RemoveWindow(window_id id)
	{
		WindowInfo& info{ GetFromId(id) };
		DestroyWindow(info.hwnd);
		RemoveFromWindows(id);
	}
#elif __APPLE__
	// OSX stuff here... open window for Metal context
#elif __linux__ // Open window for OpenGL context
	namespace {
		// Linux OS specific window info
		struct WindowInfo
		{
			XWindow*	window{ nullptr };
			Display*	display{ nullptr };
			//RECT	fullScreenArea{};
			s32			left;
			s32			top;
			s32			width;
			s32			height;

			//DWORD	style{ WS_VISIBLE };
			bool		isFullscreen{ false };
			bool		isClosed{ false };
		};

		const char* ConvertToChar(const wchar_t* text)
		{
			size_t outSize = (sizeof(text) * sizeof(wchar_t)) + 1;
			char outText[outSize];
			wcstombs(outText, text, outSize);
			return outText;
		}

		Utils::vector<WindowInfo> windows;

		/////////////////////////////////////////////////////////////////
		// TODO: this part will be handled by a free-list container later
		Utils::vector<u32> availableSlots;

		u32 AddToWindows(WindowInfo info)
		{
			u32 id{ U32_INVALID_ID };
			if (availableSlots.empty())
			{
				id = (u32)windows.size();
				windows.emplace_back(info);
			}
			else
			{
				id = availableSlots.back();
				availableSlots.pop_back();
				assert(id != U32_INVALID_ID);
				windows[id] = info;
			}

			return id;
		}

		void RemoveFromWindows(u32 id)
		{
			assert(id < windows.size());
			availableSlots.emplace_back(id);
		}
		/////////////////////////////////////////////////////////////////

		WindowInfo& GetFromId(window_id id)
		{
			assert(id < windows.size());
			assert(windows[id].window);
			return windows[id];
		}
		
		// Linux specific window class functions
		void ResizeWindow(window_id id, u32 width, u32 height)
		{
			

		}

		void SetWindowFullscreen(window_id id, bool isFullscreen)
		{

		}

		bool IsWindowFullscreen(window_id id)
		{
			return GetFromId(id).isFullscreen;
		}

		window_handle GetWindowHandle(window_id id)
		{
			return GetFromId(id).window;
		}

		void SetWindowCaption(window_id id, const wchar_t* caption)
		{
			WindowInfo& info{ GetFromId(id) };
			XStoreName(info.display, *(info.window), ConvertToChar(caption));
		}

		Math::Vec4u32 GetWindowSize(window_id id)
		{
			WindowInfo& info{ GetFromId(id) };
			return { (u32)info.left, (u32)info.top, (u32)info.width - (u32)info.left, (u32)info.height - (u32)info.top };
		}

		bool IsWindowClosed(window_id id)
		{
			return GetFromId(id).isClosed;
		}
	} // anonymous namespace

	Window MakeWindow(const WindowInitInfo* const initInfo /*= nullptr*/)
	{
		// Create display
		Display* display { XOpenDisplay(0) };
		if (display == NULL) {
			return {};
		}

		window_proc callback{ initInfo ? initInfo->callback : nullptr };
		window_handle parent{ initInfo ? initInfo->parent : &(DefaultRootWindow(display)) };

		// Setup the screen, visual, and colormap
		int screen { DefaultScreen(display) };
		Visual* visual { DefaultVisual(display, screen) };
		Colormap colormap { XCreateColormap(display, DefaultRootWindow(display), visual, AllocNone) };

		// Define attributes for the window
		XSetWindowAttributes attributes;
		attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
								ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
		attributes.colormap = colormap;

		// Create an instance of WindowInfo
		WindowInfo info{};
		info.left = (initInfo && initInfo->left) ? initInfo->left : 0;	// generally, the X window manager overrides
		info.top = (initInfo && initInfo->top) ? initInfo->top : 0;		// the starting top left coords, so default is 0,0
		info.width = (initInfo && initInfo->width) ? initInfo->width : DisplayWidth(display, DefaultScreen(display));
		info.height = (initInfo && initInfo->height) ? initInfo->height : DisplayHeight(display, DefaultScreen(display));
		info.display = display;

		// check for initial info, use defaults if none given
		const wchar_t* caption{ (initInfo && initInfo->caption) ? initInfo->caption : L"Havana Game" };

		XWindow window { XCreateWindow(display, *parent, info.left, info.top, info.width, info.height, 0,
										DefaultDepth(display, screen), InputOutput, visual,
										CWColormap | CWEventMask, &attributes) };
		info.window = &window;

		// Create_the_modern_OpenGL_context
		static int visualAttribs[] {
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
			GLX_DOUBLEBUFFER, true,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			None
		};

		int numFBC { 0 };
		GLXFBConfig *fbc { glXChooseFBConfig(display,
											DefaultScreen(display),
											visualAttribs, &numFBC) };
		if (!fbc) {
			return {};
		}

		glXCreateContextAttribsARBProc glXCreateContextAttribsARB { 0 };
		glXCreateContextAttribsARB =
			(glXCreateContextAttribsARBProc)
			glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

		if (!glXCreateContextAttribsARB) {
			return {};
		}

		// Set desired minimum OpenGL version
		int contextAttribs[] {
			GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
			GLX_CONTEXT_MINOR_VERSION_ARB, 2,
			None
		};

		// Create modern OpenGL context
		GLXContext context { glXCreateContextAttribsARB(display, fbc[0], NULL, true,
													contextAttribs) };
		if (!context) {
			return {};
		}

		// Show window
		XMapWindow(display, window);
		XStoreName(display, window, "Modern GLX with X11");
		glXMakeCurrent(display, window, context);

		int major { 0 }, minor { 0 };
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
	}

	void RemoveWindow(window_id id)
	{
		WindowInfo& info{ GetFromId(id) };
		XDestroyWindow(info.display, *(info.window));
    	XCloseDisplay(info.display);
		RemoveFromWindows(id);
	}
	
#elif
#error Must implement at least one platform.
#endif

	//***************************************************************
	// Implementation of the Window class abstraction layer functions
	//***************************************************************
	
	void Window::SetFullscreen(bool isFullscreen) const
	{
		assert(IsValid());
		SetWindowFullscreen(m_id, isFullscreen);
	}

	bool Window::IsFullscreen() const
	{
		assert(IsValid());
		return IsWindowFullscreen(m_id);
	}

	void* Window::Handle() const
	{
		assert(IsValid());
		return GetWindowHandle(m_id);
	}

	void Window::SetCaption(const wchar_t* caption) const
	{
		assert(IsValid());
		SetWindowCaption(m_id, caption);
	}

	Math::Vec4u32 Window::Size() const
	{
		assert(IsValid());
		return GetWindowSize(m_id);
	}

	void Window::Resize(u32 width, u32 height) const
	{
		assert(IsValid());
		ResizeWindow(m_id, width, height);
	}

	u32 Window::Width() const
	{
		Math::Vec4u32 size{ Size() };
		return size.z - size.x;
	}

	u32 Window::Height() const
	{
		Math::Vec4u32 size{ Size() };
		return size.w - size.y;
	}

	bool Window::IsClosed() const
	{
		assert(IsValid());
		return IsWindowClosed(m_id);
	}

}