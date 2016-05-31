
#include "SignalHandlers.h"
#include "System.h"

#ifdef WIN32
	#include <Windows.h>
	#include <thread>

	static std::thread threadMessageLoop;
#endif

#include <csignal>

static HttpServer::Server *globalServerPtr = nullptr;

/**
 * Terminate signal
 */
static void handlerSigTerm(const int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->stopProcess();
	}
}

/**
 * Interrupt signal
 */
static void handlerSigInt(const int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->stopProcess();
	}
}

/**
 * Signal to restart
 */
static void handlerSigUsr1(const int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->setRestart();
		globalServerPtr->stopProcess();
	}
}

/**
 * Signal to update modules
 */
static void handlerSigUsr2(const int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->setUpdateModule();
		globalServerPtr->unsetProcess();
		globalServerPtr->setProcessQueue();
	}
}

#ifdef WIN32
/**
 * Note: PostQuitMessage(0)
 *  It doesn't work in case the program was launched and was
 *  attempted to finish under different remote sessions.
 */
static ::LRESULT CALLBACK WndProc(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, const ::LPARAM lParam)
{
	switch (message)
	{
		case SIGTERM:
		case WM_CLOSE:
		{
			handlerSigTerm(message);
			::PostMessage(hWnd, WM_QUIT, 0, 0); // Fuck ::PostQuitMessage(0);

			break;
		}

		case SIGINT:
		{
			handlerSigInt(message);
			::PostMessage(hWnd, WM_QUIT, 0, 0); // Fuck ::PostQuitMessage(0);

			break;
		}

		case SIGUSR1:
		{
			handlerSigUsr1(message);
			break;
		}

		case SIGUSR2:
		{
			handlerSigUsr2(message);
			break;
		}

		default:
		{
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

static ::WPARAM mainMessageLoop(const ::HINSTANCE hInstance, HttpServer::Event * const eventWindowCreation)
{
	const ::HWND hWnd = ::CreateWindow(myWndClassName, nullptr, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, nullptr);

	eventWindowCreation->notify(); // After this action, eventWindowCreation will be destroyed (in the other thread)

	if (0 == hWnd)
	{
		return 0;
	}

	::MSG msg;

	while (::GetMessage(&msg, hWnd, 0, 0) )
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return msg.wParam;
}

#ifdef _CONSOLE
static ::BOOL consoleSignalHandler(const ::DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_CLOSE_EVENT:
		handlerSigTerm(ctrlType);
		std::this_thread::sleep_for(std::chrono::seconds(60) );
		return true;

	case CTRL_C_EVENT:
		handlerSigInt(ctrlType);
		return true;

	default:
		return false;
	}
}
#endif // _CONSOLE
#endif // WIN32

bool bindSignalHandlers(HttpServer::Server *server)
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

	::WNDCLASSEX wcex = {};

	wcex.cbSize = sizeof(::WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = myWndClassName;

	if (0 == ::RegisterClassEx(&wcex) )
	{
		return false;
	}

	HttpServer::Event eventWindowCreation;

	threadMessageLoop = std::thread(mainMessageLoop, hInstance, &eventWindowCreation);

	eventWindowCreation.wait();

#elif POSIX

	struct ::sigaction act = {};

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
	#error "Undefine platform"
#endif

	return true;
}

void stopSignalHandlers()
{
#ifdef WIN32
	System::sendSignal(::GetCurrentProcessId(), SIGINT);
	threadMessageLoop.join();
#endif
}