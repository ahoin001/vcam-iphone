#pragma once

namespace usbmux
{
	// Listens on 127.0.0.1:<port> and tunnels each connection to the iPhone
	// through Apple Mobile Device Service (usbmuxd on 127.0.0.1:27015).
	// Returns when requestStop() runs. Closing the listener removes the mapping.
	int run(int port);

	void requestStop();
}
