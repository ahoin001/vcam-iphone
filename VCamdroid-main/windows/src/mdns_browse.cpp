#include "mdns_browse.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace mdns
{
	namespace
	{
		constexpr const char* kService = "_droidcamobs._tcp.local";

		void writeName(std::vector<uint8_t>& packet, const std::string& name)
		{
			size_t start = 0;
			while (start < name.size())
			{
				size_t dot = name.find('.', start);
				if (dot == std::string::npos)
				{
					dot = name.size();
				}
				const uint8_t length = static_cast<uint8_t>(dot - start);
				packet.push_back(length);
				packet.insert(packet.end(), name.begin() + static_cast<std::ptrdiff_t>(start), name.begin() + static_cast<std::ptrdiff_t>(dot));
				start = dot + 1;
			}
			packet.push_back(0);
		}

		bool readName(const uint8_t* data, size_t length, size_t& cursor, std::string& name, int depth = 0)
		{
			if (depth > 8 || cursor >= length)
			{
				return false;
			}
			while (cursor < length)
			{
				const uint8_t label = data[cursor];
				if (label == 0)
				{
					cursor += 1;
					if (!name.empty() && name.back() == '.')
					{
						name.pop_back();
					}
					return true;
				}
				if ((label & 0xC0) == 0xC0)
				{
					if (cursor + 1 >= length)
					{
						return false;
					}
					const size_t jump = static_cast<size_t>(((label & 0x3F) << 8) | data[cursor + 1]);
					cursor += 2;
					size_t nested = jump;
					return readName(data, length, nested, name, depth + 1);
				}
				if ((label & 0xC0) != 0 || cursor + 1 + label > length)
				{
					return false;
				}
				if (!name.empty())
				{
					name.push_back('.');
				}
				name.append(reinterpret_cast<const char*>(data + cursor + 1), label);
				cursor += 1u + label;
			}
			return false;
		}

		uint16_t readU16(const uint8_t* data, size_t length, size_t& cursor)
		{
			if (cursor + 2 > length)
			{
				cursor = length;
				return 0;
			}
			const uint16_t value = static_cast<uint16_t>((data[cursor] << 8) | data[cursor + 1]);
			cursor += 2;
			return value;
		}

		std::string lower(std::string text)
		{
			for (char& unit : text)
			{
				if (unit >= 'A' && unit <= 'Z')
				{
					unit = static_cast<char>(unit - 'A' + 'a');
				}
			}
			return text;
		}

		std::string instanceName(const std::string& full)
		{
			const std::string suffix = "._droidcamobs._tcp.local";
			const std::string lowered = lower(full);
			if (lowered.size() > suffix.size() && lowered.compare(lowered.size() - suffix.size(), suffix.size(), suffix) == 0)
			{
				return full.substr(0, full.size() - suffix.size());
			}
			return full;
		}

		std::string txtName(const uint8_t* data, size_t length)
		{
			size_t cursor = 0;
			while (cursor < length)
			{
				const uint8_t size = data[cursor++];
				if (size == 0 || cursor + size > length)
				{
					break;
				}
				const std::string pair(reinterpret_cast<const char*>(data + cursor), size);
				cursor += size;
				if (pair.rfind("name=", 0) == 0)
				{
					return pair.substr(5);
				}
			}
			return {};
		}

		struct Records
		{
			std::map<std::string, std::string> ptrToHost;
			std::map<std::string, int> hostPort;
			std::map<std::string, std::string> hostIpv4;
			std::map<std::string, std::string> ptrTxt;
		};

		void parsePacket(const uint8_t* data, size_t length, Records& records)
		{
			if (length < 12)
			{
				return;
			}
			size_t cursor = 4;
			const uint16_t questions = readU16(data, length, cursor);
			const uint16_t answers = readU16(data, length, cursor);
			const uint16_t authority = readU16(data, length, cursor);
			const uint16_t additional = readU16(data, length, cursor);
			for (uint16_t index = 0; index < questions; ++index)
			{
				std::string name;
				if (!readName(data, length, cursor, name))
				{
					return;
				}
				cursor += 4;
			}
			const uint16_t total = static_cast<uint16_t>(answers + authority + additional);
			for (uint16_t index = 0; index < total; ++index)
			{
				std::string name;
				if (!readName(data, length, cursor, name))
				{
					return;
				}
				const uint16_t type = readU16(data, length, cursor);
				cursor += 2;
				cursor += 4;
				const uint16_t rdlength = readU16(data, length, cursor);
				if (cursor + rdlength > length)
				{
					return;
				}
				const uint8_t* rdata = data + cursor;
				const std::string key = lower(name);
				if (type == 12)
				{
					size_t nested = cursor;
					std::string target;
					if (readName(data, length, nested, target))
					{
						records.ptrToHost[key] = target;
					}
				}
				else if (type == 33 && rdlength >= 6)
				{
					const int port = (rdata[4] << 8) | rdata[5];
					size_t nested = cursor + 6;
					std::string target;
					if (readName(data, length, nested, target))
					{
						records.hostPort[key] = port;
						records.ptrToHost[key] = target;
					}
				}
				else if (type == 1 && rdlength == 4)
				{
					records.hostIpv4[key] = std::to_string(rdata[0]) + "." + std::to_string(rdata[1]) + "."
						+ std::to_string(rdata[2]) + "." + std::to_string(rdata[3]);
				}
				else if (type == 16)
				{
					const std::string label = txtName(rdata, rdlength);
					if (!label.empty())
					{
						records.ptrTxt[key] = label;
					}
				}
				cursor += rdlength;
			}
		}
	}

	std::vector<Service> browse(int timeoutMs)
	{
		std::vector<Service> found;
		SOCKET socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (socket == INVALID_SOCKET)
		{
			return found;
		}
		BOOL reuse = TRUE;
		setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
		DWORD timeout = timeoutMs > 0 ? static_cast<DWORD>(timeoutMs) : 1200;
		setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
		sockaddr_in local{};
		local.sin_family = AF_INET;
		local.sin_port = 0;
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(socket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0)
		{
			closesocket(socket);
			return found;
		}
		ip_mreq membership{};
		inet_pton(AF_INET, "224.0.0.251", &membership.imr_multiaddr);
		membership.imr_interface.s_addr = htonl(INADDR_ANY);
		setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&membership), sizeof(membership));

		std::vector<uint8_t> query(12, 0);
		query[5] = 1;
		writeName(query, kService);
		query.push_back(0);
		query.push_back(12);
		query.push_back(0);
		query.push_back(1);

		sockaddr_in multicast{};
		multicast.sin_family = AF_INET;
		multicast.sin_port = htons(5353);
		inet_pton(AF_INET, "224.0.0.251", &multicast.sin_addr);
		sendto(socket, reinterpret_cast<const char*>(query.data()), static_cast<int>(query.size()), 0,
			reinterpret_cast<sockaddr*>(&multicast), sizeof(multicast));

		Records records;
		const ULONGLONG deadline = GetTickCount64() + timeout;
		uint8_t buffer[2048];
		while (GetTickCount64() < deadline)
		{
			sockaddr_in from{};
			int fromSize = sizeof(from);
			const int read = recvfrom(socket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0,
				reinterpret_cast<sockaddr*>(&from), &fromSize);
			if (read <= 0)
			{
				continue;
			}
			parsePacket(buffer, static_cast<size_t>(read), records);
		}
		closesocket(socket);

		std::map<std::string, Service> unique;
		for (const auto& portEntry : records.hostPort)
		{
			Service service;
			service.port = portEntry.second == 0 ? 4747 : portEntry.second;
			const std::string instance = portEntry.first;
			auto host = records.ptrToHost.find(instance);
			std::string hostname = host != records.ptrToHost.end() ? lower(host->second) : instance;
			auto ipv4 = records.hostIpv4.find(hostname);
			if (ipv4 == records.hostIpv4.end())
			{
				ipv4 = records.hostIpv4.find(instance);
			}
			if (ipv4 == records.hostIpv4.end())
			{
				continue;
			}
			service.ipv4 = ipv4->second;
			auto txt = records.ptrTxt.find(instance);
			if (txt != records.ptrTxt.end())
			{
				service.name = txt->second;
			}
			else
			{
				service.name = instanceName(instance);
			}
			if (service.name.empty())
			{
				service.name = "VCamdroid";
			}
			unique[service.ipv4 + ":" + std::to_string(service.port)] = service;
		}
		for (const auto& entry : unique)
		{
			found.push_back(entry.second);
		}
		return found;
	}
}
