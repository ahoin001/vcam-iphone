#include "usbmux.h"

#include <windows.h>

#include <cstdlib>
#include <iostream>

namespace
{
	BOOL WINAPI consoleHandler(DWORD)
	{
		usbmux::requestStop();
		return TRUE;
	}
}

int main(int argc, char** argv)
{
	const int port = argc > 1 ? std::atoi(argv[1]) : 4747;
	if (port <= 0 || port > 65535)
	{
		std::cerr << "[USBMUX] Port must be between 1 and 65535." << std::endl;
		return 1;
	}
	SetConsoleCtrlHandler(consoleHandler, TRUE);
	return usbmux::run(port);
}
