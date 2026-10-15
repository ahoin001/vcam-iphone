#pragma once

#include <string>
#include <vector>

namespace usbmux
{
	struct Device
	{
		int id = 0;
		std::string serial;
		std::string name;
	};

	struct ListResult
	{
		std::vector<Device> devices;
		std::string error;
	};

	ListResult listDevices();

	// Listens on 127.0.0.1:<port> in a background thread and tunnels each
	// connection to the iPhone. deviceId 0 uses the first attached phone.
	bool start(int port, int deviceId, std::string& error);
	void stop();
	bool isRunning();
	int listeningPort();

	// Console helper: start(), then block until requestStop().
	int run(int port);
	void requestStop();
}
