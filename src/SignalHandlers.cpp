
#include "SignalHandlers.h"
#include "system/System.h"

#ifdef WIN32
	#include <Windows.h>
	#include <thread>

	static DWORD gMainThreadId;
	static std::thread gThreadMessageLoop;
#endif

#include <csignal>

static HttpServer::Server *globalServerPtr = nullptr;

/**
 * Terminate signal
 */
static void handlerSigTerm(const int) noexcept {
	if (globalServerPtr) {
		globalServerPtr->stop();
	}
}

/**
 * Interrupt signal
 */
static void handlerSigInt(const int) noexcept {
	if (globalServerPtr) {
		globalServerPtr->stop();
	}
}

/**
 * Signal to restart
 */
static void handlerSigUsr1(const int) noexcept {
	if (globalServerPtr) {
		globalServerPtr->restart();
	}
}

/**
 * Signal to update modules
 */
static void handlerSigUsr2(const int) noexcept {
	if (globalServerPtr) {
		globalServerPtr->update();
	}
}

#ifdef WIN32
/**
 * Note: PostQuitMessage(0)
 *  It doesn't work in case the program was launched and was
 *  attempted to finish under different remote sessions.
 */
static ::LRESULT CALLBACK WndProc(
	const ::HWND hWnd,
	const ::UINT message,
	const ::WPARAM wParam,
	const ::LPARAM lParam
) noexcept {
	switch (message)
	{
		case SIGTERM: {
			handlerSigTerm(message);
			::PostMessage(hWnd, WM_QUIT, 0, 0); // Fuck ::PostQuitMessage(0);

			break;
		}

		case SIGINT: {
			handlerSigInt(message);
			::PostMessage(hWnd, WM_QUIT, 0, 0); // Fuck ::PostQuitMessage(0);

			break;
		}

		case SIGUSR1: {
			handlerSigUsr1(message);
			break;
		}

		case SIGUSR2: {
			handlerSigUsr2(message);
			break;
		}

		// Cases WM_QUERYENDSESSION and WM_ENDSESSION run before shutting down the system (or ending user session)
		case WM_QUERYENDSESSION: {
			handlerSigTerm(message);
			break;
		}

		case WM_ENDSESSION: {
			const ::HANDLE hThread = ::OpenThread(SYNCHRONIZE, false, gMainThreadId);
			::WaitForSingleObject(hThread, INFINITE);
			::CloseHandle(hThread);
			break;
		}

		default: {
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

static ::WPARAM mainMessageLoop(
	const ::HINSTANCE hInstance,
	Utils::Event * const eventWindowCreation
) noexcept {
	const ::HWND hWnd = ::CreateWindow(
		myWndClassName,
		nullptr,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	eventWindowCreation->notify(); // After this action, eventWindowCreation will be destroyed (in the other thread)

	if (0 == hWnd) {
		return 0;
	}

	::MSG msg;

	while (::GetMessage(&msg, hWnd, 0, 0) ) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return msg.wParam;
}

#ifdef _CONSOLE
static ::BOOL consoleSignalHandler(const ::DWORD ctrlType) noexcept
{
	switch (ctrlType)
	{
	case CTRL_CLOSE_EVENT:
	// Cases CTRL_LOGOFF_EVENT and CTRL_SHUTDOWN_EVENT don't happen because... it's Windows %)
	//  @see my function WndProc -> cases WM_QUERYENDSESSION and WM_ENDSESSION. Only they happen in this program, because the library user32.dll is connected.
	//  @prooflink: https://msdn.microsoft.com/library/windows/desktop/ms686016(v=vs.85).aspx
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT: {
		handlerSigTerm(ctrlType);
		const ::HANDLE hThread = ::OpenThread(SYNCHRONIZE, false, gMainThreadId);
		::WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		return true;
	}

	case CTRL_C_EVENT: {
		handlerSigInt(ctrlType);
		return true;
	}

	default:
		return false;
	}
}
#endif // _CONSOLE
#endif // WIN32

bool bindSignalHandlers(HttpServer::Server *server) noexcept
{
	globalServerPtr = server;

#ifdef WIN32

#ifdef _CONSOLE
	::SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(consoleSignalHandler), true);
#elif
	::signal(SIGINT, handlerSigInt);
	::signal(SIGTERM, handlerSigTerm);
#endif // _CONSOLE

	const ::HINSTANCE hInstance = ::GetModuleHandle(nullptr);

	::WNDCLASSEX wcex {};

	wcex.cbSize = sizeof(::WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = myWndClassName;

	if (::RegisterClassEx(&wcex) == 0) {
		return false;
	}

	Utils::Event eventWindowCreation;

	gMainThreadId = ::GetCurrentThreadId();
	gThreadMessageLoop = std::thread(mainMessageLoop, hInstance, &eventWindowCreation);

	eventWindowCreation.wait();

#elif POSIX

	struct ::sigaction act {};

	act.sa_handler = handlerSigInt;
	::sigaction(SIGINT, &act, nullptr);

	act.sa_handler = handlerSigTerm;
	::sigaction(SIGTERM, &act, nullptr);

	act.sa_handler = handlerSigUsr1;
	::sigaction(SIGUSR1, &act, nullptr);

	act.sa_handler = handlerSigUsr2;
	::sigaction(SIGUSR2, &act, nullptr);

	::signal(SIGPIPE, SIG_IGN);

#else
	#error "Undefined platform"
#endif

	return true;
}

void stopSignalHandlers() noexcept
{
#ifdef WIN32
	System::sendSignal(::GetCurrentProcessId(), SIGINT);
	gThreadMessageLoop.join();
#endif
}
