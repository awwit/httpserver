
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

#ifdef WIN32

#ifndef SIGUSR1
	#define SIGUSR1 10
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case SIGTERM:
		{
			handlerSigTerm(message);
			PostQuitMessage(0);

			break;
		}

		case SIGINT:
		{
			handlerSigInt(message);
			PostQuitMessage(0);

			break;
		}

		case SIGUSR1:
		{
			handlerSigUsr1(message);
			break;
		}

		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

int mainMessageLoop(HINSTANCE hInstance, HttpServer::Event *pCreatedWindow)
{
	HWND hWnd = CreateWindow(wndClassName, nullptr, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, nullptr);

	pCreatedWindow->notify();

	if (0 == hWnd)
	{
		return 0;
	}

	MSG msg;

	while (GetMessage(&msg, hWnd, 0, 0) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
#endif

int bindSignalsHandles(HttpServer::Server *server)
{
	globalServerPtr = server;

#ifdef WIN32

	signal(SIGINT, handlerSigInt);
	signal(SIGTERM, handlerSigTerm);

	_set_abort_behavior(0, _WRITE_ABORT_MSG);

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wcex = {0};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = wndClassName;

	if (0 == RegisterClassEx(&wcex) )
	{
		return 0;
	}

	HttpServer::Event createdWindow;

	threadMessageLoop = std::thread(mainMessageLoop, hInstance, &createdWindow);

	createdWindow.wait();

	return 1;

#elif POSIX

	struct sigaction act = {0};

	act.sa_handler = handlerSigInt;
	sigaction(SIGINT, &act, nullptr);

	act.sa_handler = handlerSigTerm;
	sigaction(SIGTERM, &act, nullptr);

	act.sa_handler = handlerSigUsr1;
	sigaction(SIGUSR1, &act, nullptr);

	return 1;
#else
	#error "Undefine platform"
#endif
}