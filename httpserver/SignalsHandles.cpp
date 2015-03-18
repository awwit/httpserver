
#include "SignalsHandles.h"

void handlerSigTerm(int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->stopProcess();
	}
}

void handlerSigInt(int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->stopProcess();
	}
}

void handlerSigUsr1(int sig)
{
	if (globalServerPtr)
	{
		globalServerPtr->setRestart();
		globalServerPtr->stopProcess();
	}
}

void handlerSigUsr2(int sig)
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
::LRESULT CALLBACK WndProc(::HWND hWnd, ::UINT message, ::WPARAM wParam, ::LPARAM lParam)
{
	switch (message)
	{
		case SIGTERM:
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

::WPARAM mainMessageLoop(::HINSTANCE hInstance, HttpServer::Event *pCreatedWindow)
{
    ::HWND hWnd = ::CreateWindow(myWndClassName, nullptr, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, nullptr);

	pCreatedWindow->notify();

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
#endif

int bindSignalsHandles(HttpServer::Server *server)
{
	globalServerPtr = server;

#ifdef WIN32

    const int sig_int = 2; // SIGINT
    const int sig_term = 15; // SIGTERM

    ::signal(sig_int, handlerSigInt);
    ::signal(sig_term, handlerSigTerm);

    ::_set_abort_behavior(0, _WRITE_ABORT_MSG);

    ::HINSTANCE hInstance = ::GetModuleHandle(nullptr);

    ::WNDCLASSEX wcex = {
        sizeof(::WNDCLASSEX)
	};

	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = myWndClassName;

    if (0 == ::RegisterClassEx(&wcex) )
	{
		return 0;
	}

	HttpServer::Event createdWindow;

	threadMessageLoop = std::thread(mainMessageLoop, hInstance, &createdWindow);

	createdWindow.wait();

#elif POSIX

    struct ::sigaction act = {0};

	act.sa_handler = handlerSigInt;
    ::sigaction(SIGINT, &act, nullptr);

	act.sa_handler = handlerSigTerm;
    ::sigaction(SIGTERM, &act, nullptr);

	act.sa_handler = handlerSigUsr1;
    ::sigaction(SIGUSR1, &act, nullptr);

	act.sa_handler = handlerSigUsr2;
    ::sigaction(SIGUSR2, &act, nullptr);
#else
	#error "Undefine platform"
#endif

	return 1;
}