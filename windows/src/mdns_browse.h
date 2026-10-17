#pragma once

#include <string>
#include <vector>

namespace mdns
{
	struct Service
	{
		std::string name;
		std::string ipv4;
		int port = 4747;
	};

	std::vector<Service> browse(int timeoutMs = 1200);
}
