#include "usbmux.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace usbmux
{
	namespace
	{
		constexpr int kUsbmuxPort = 27015;
		constexpr uint32_t kPlistMessage = 8;

		std::atomic<bool> gStop{ false };
		std::atomic<bool> gRunning{ false };
		std::atomic<bool> gLog{ false };
		SOCKET gListen = INVALID_SOCKET;
		std::mutex gThreadsMutex;
		std::vector<std::thread> gThreads;
		std::thread gAcceptThread;
		bool gOwnsWSA = false;
		int gDeviceID = 0;
		int gPort = 4747;

		#pragma pack(push, 1)
		struct Header
		{
			uint32_t length;
			uint32_t version;
			uint32_t message;
			uint32_t tag;
		};
		#pragma pack(pop)

		void log(int port, const std::string& message)
		{
			if (!gLog.load())
			{
				return;
			}
			std::cout << "[USBMUX:" << port << "] " << message << std::endl;
		}

		bool sendAll(SOCKET socket, const char* data, int length)
		{
			int sent = 0;
			while (sent < length)
			{
				const int wrote = ::send(socket, data + sent, length - sent, 0);
				if (wrote <= 0)
				{
					return false;
				}
				sent += wrote;
			}
			return true;
		}

		bool recvAll(SOCKET socket, char* data, int length)
		{
			int got = 0;
			while (got < length)
			{
				const int read = ::recv(socket, data + got, length - got, 0);
				if (read <= 0)
				{
					return false;
				}
				got += read;
			}
			return true;
		}

		void setNoDelay(SOCKET socket)
		{
			BOOL enabled = TRUE;
			setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&enabled), sizeof(enabled));
		}

		uint64_t readBe(const std::vector<uint8_t>& bytes, size_t offset, int size)
		{
			uint64_t value = 0;
			for (int index = 0; index < size; ++index)
			{
				value = (value << 8) | bytes[offset + static_cast<size_t>(index)];
			}
			return value;
		}

		struct PlistObject
		{
			enum class Kind
			{
				Null,
				Bool,
				Int,
				String,
				Array,
				Dict
			};

			Kind kind = Kind::Null;
			bool boolean = false;
			int64_t integer = 0;
			std::string text;
			std::vector<uint64_t> refs;
		};

		class BinaryPlist
		{
		public:
			bool parse(const uint8_t* data, size_t length)
			{
				if (length < 32 + 8 || std::memcmp(data, "bplist00", 8) != 0)
				{
					return false;
				}
				bytes.assign(data, data + length);
				const size_t trailer = length - 32;
				const uint8_t offsetSize = bytes[trailer + 6];
				refSize = bytes[trailer + 7];
				const uint64_t count = readBe(bytes, trailer + 8, 8);
				rootIndex = readBe(bytes, trailer + 16, 8);
				const uint64_t table = readBe(bytes, trailer + 24, 8);
				if (offsetSize == 0 || refSize == 0 || count == 0 || table >= length)
				{
					return false;
				}
				offsets.resize(static_cast<size_t>(count));
				objects.resize(static_cast<size_t>(count));
				decoded.assign(static_cast<size_t>(count), false);
				for (uint64_t index = 0; index < count; ++index)
				{
					offsets[static_cast<size_t>(index)] = readBe(bytes, static_cast<size_t>(table + index * offsetSize), offsetSize);
				}
				return decode(rootIndex);
			}

			std::optional<int64_t> findInt(const std::string& key) const
			{
				if (rootIndex >= objects.size())
				{
					return std::nullopt;
				}
				std::optional<int64_t> found;
				walk(objects[static_cast<size_t>(rootIndex)], key, found);
				return found;
			}

			std::vector<Device> devices() const
			{
				std::vector<Device> found;
				if (rootIndex < objects.size())
				{
					collect(objects[static_cast<size_t>(rootIndex)], found);
				}
				return found;
			}

		private:
			bool decode(uint64_t index)
			{
				if (index >= objects.size() || decoded[static_cast<size_t>(index)])
				{
					return index < objects.size();
				}
				decoded[static_cast<size_t>(index)] = true;
				size_t cursor = static_cast<size_t>(offsets[static_cast<size_t>(index)]);
				if (cursor >= bytes.size())
				{
					return false;
				}
				const uint8_t marker = bytes[cursor++];
				PlistObject& object = objects[static_cast<size_t>(index)];
				const uint8_t type = marker & 0xF0;
				if (type == 0x00)
				{
					object.kind = marker == 0x09 || marker == 0x08 ? PlistObject::Kind::Bool : PlistObject::Kind::Null;
					object.boolean = marker == 0x09;
					return true;
				}
				if (type == 0x10)
				{
					const int size = 1 << (marker & 0x0F);
					if (cursor + static_cast<size_t>(size) > bytes.size())
					{
						return false;
					}
					object.kind = PlistObject::Kind::Int;
					object.integer = static_cast<int64_t>(readBe(bytes, cursor, size));
					return true;
				}
				if (type == 0x50 || type == 0x60 || type == 0xA0 || type == 0xD0)
				{
					const uint64_t count = readCount(cursor, marker);
					if (type == 0x50)
					{
						if (cursor + count > bytes.size())
						{
							return false;
						}
						object.kind = PlistObject::Kind::String;
						object.text.assign(reinterpret_cast<const char*>(bytes.data() + cursor), static_cast<size_t>(count));
						return true;
					}
					if (type == 0x60)
					{
						if (cursor + count * 2 > bytes.size())
						{
							return false;
						}
						object.kind = PlistObject::Kind::String;
						for (uint64_t unit = 0; unit < count; ++unit)
						{
							const uint16_t code = static_cast<uint16_t>(readBe(bytes, cursor + static_cast<size_t>(unit * 2), 2));
							if (code < 0x80)
							{
								object.text.push_back(static_cast<char>(code));
							}
						}
						return true;
					}
					const size_t refCount = type == 0xD0 ? static_cast<size_t>(count * 2) : static_cast<size_t>(count);
					if (cursor + refCount * refSize > bytes.size())
					{
						return false;
					}
					object.kind = type == 0xD0 ? PlistObject::Kind::Dict : PlistObject::Kind::Array;
					object.refs.resize(refCount);
					for (size_t ref = 0; ref < refCount; ++ref)
					{
						object.refs[ref] = readBe(bytes, cursor + ref * refSize, static_cast<int>(refSize));
						if (!decode(object.refs[ref]))
						{
							return false;
						}
					}
					return true;
				}
				return true;
			}

			uint64_t readCount(size_t& cursor, uint8_t marker)
			{
				const uint8_t nibble = marker & 0x0F;
				if (nibble != 0x0F)
				{
					return nibble;
				}
				if (cursor >= bytes.size())
				{
					return 0;
				}
				const uint8_t sizeMarker = bytes[cursor++];
				const int size = 1 << (sizeMarker & 0x0F);
				if (cursor + static_cast<size_t>(size) > bytes.size())
				{
					return 0;
				}
				const uint64_t count = readBe(bytes, cursor, size);
				cursor += static_cast<size_t>(size);
				return count;
			}

			void walk(const PlistObject& object, const std::string& key, std::optional<int64_t>& found) const
			{
				if (found)
				{
					return;
				}
				if (object.kind == PlistObject::Kind::Dict)
				{
					for (size_t index = 0; index + 1 < object.refs.size(); index += 2)
					{
						const PlistObject* name = at(object.refs[index]);
						const PlistObject* value = at(object.refs[index + 1]);
						if (name != nullptr && value != nullptr && name->kind == PlistObject::Kind::String && name->text == key && value->kind == PlistObject::Kind::Int)
						{
							found = value->integer;
							return;
						}
						if (value != nullptr)
						{
							walk(*value, key, found);
						}
					}
				}
				else if (object.kind == PlistObject::Kind::Array)
				{
					for (const uint64_t ref : object.refs)
					{
						if (const PlistObject* child = at(ref))
						{
							walk(*child, key, found);
						}
					}
				}
			}

			const PlistObject* at(uint64_t index) const
			{
				if (index >= objects.size())
				{
					return nullptr;
				}
				return &objects[static_cast<size_t>(index)];
			}

			const PlistObject* dictValue(const PlistObject& dict, const std::string& key) const
			{
				if (dict.kind != PlistObject::Kind::Dict)
				{
					return nullptr;
				}
				for (size_t index = 0; index + 1 < dict.refs.size(); index += 2)
				{
					const PlistObject* name = at(dict.refs[index]);
					if (name != nullptr && name->kind == PlistObject::Kind::String && name->text == key)
					{
						return at(dict.refs[index + 1]);
					}
				}
				return nullptr;
			}

			std::string dictString(const PlistObject& dict, const std::string& key) const
			{
				if (const PlistObject* value = dictValue(dict, key); value != nullptr && value->kind == PlistObject::Kind::String)
				{
					return value->text;
				}
				return {};
			}

			void collect(const PlistObject& object, std::vector<Device>& found) const
			{
				if (object.kind == PlistObject::Kind::Array)
				{
					for (const uint64_t ref : object.refs)
					{
						if (const PlistObject* child = at(ref))
						{
							collect(*child, found);
						}
					}
					return;
				}
				if (object.kind != PlistObject::Kind::Dict)
				{
					return;
				}
				if (const PlistObject* idValue = dictValue(object, "DeviceID"); idValue != nullptr && idValue->kind == PlistObject::Kind::Int)
				{
					Device device;
					device.id = static_cast<int>(idValue->integer);
					device.serial = dictString(object, "SerialNumber");
					device.name = dictString(object, "DeviceName");
					if (const PlistObject* properties = dictValue(object, "Properties"); properties != nullptr && properties->kind == PlistObject::Kind::Dict)
					{
						if (device.serial.empty())
						{
							device.serial = dictString(*properties, "SerialNumber");
						}
						if (device.name.empty())
						{
							device.name = dictString(*properties, "DeviceName");
						}
					}
					if (device.name.empty())
					{
						device.name = "iPhone";
					}
					found.push_back(device);
					return;
				}
				for (size_t index = 0; index + 1 < object.refs.size(); index += 2)
				{
					if (const PlistObject* value = at(object.refs[index + 1]))
					{
						collect(*value, found);
					}
				}
			}

			std::vector<uint8_t> bytes;
			std::vector<uint64_t> offsets;
			std::vector<PlistObject> objects;
			std::vector<bool> decoded;
			uint64_t rootIndex = 0;
			uint8_t refSize = 0;
		};

		std::optional<int64_t> xmlInteger(const std::string& xml, const std::string& key)
		{
			const std::string needle = "<key>" + key + "</key>";
			const size_t keyAt = xml.find(needle);
			if (keyAt == std::string::npos)
			{
				return std::nullopt;
			}
			const size_t valueAt = xml.find("<integer>", keyAt);
			const size_t end = xml.find("</integer>", valueAt == std::string::npos ? keyAt : valueAt);
			if (valueAt == std::string::npos || end == std::string::npos || valueAt > end)
			{
				return std::nullopt;
			}
			const std::string digits = xml.substr(valueAt + 9, end - (valueAt + 9));
			char* parsedEnd = nullptr;
			const long long value = std::strtoll(digits.c_str(), &parsedEnd, 10);
			if (parsedEnd == digits.c_str())
			{
				return std::nullopt;
			}
			return static_cast<int64_t>(value);
		}

		std::optional<std::string> xmlString(const std::string& xml, const std::string& key)
		{
			const std::string needle = "<key>" + key + "</key>";
			const size_t keyAt = xml.find(needle);
			if (keyAt == std::string::npos)
			{
				return std::nullopt;
			}
			const size_t valueAt = xml.find("<string>", keyAt);
			const size_t end = xml.find("</string>", valueAt == std::string::npos ? keyAt : valueAt);
			if (valueAt == std::string::npos || end == std::string::npos || valueAt > end)
			{
				return std::nullopt;
			}
			return xml.substr(valueAt + 8, end - (valueAt + 8));
		}

		std::vector<Device> xmlDevices(const std::string& xml)
		{
			std::vector<Device> found;
			const std::string needle = "<key>DeviceID</key>";
			size_t pos = 0;
			while ((pos = xml.find(needle, pos)) != std::string::npos)
			{
				const size_t next = xml.find(needle, pos + needle.size());
				const std::string region = xml.substr(pos, (next == std::string::npos ? xml.size() : next) - pos);
				const std::optional<int64_t> id = xmlInteger(region, "DeviceID");
				if (id)
				{
					Device device;
					device.id = static_cast<int>(*id);
					device.serial = xmlString(region, "SerialNumber").value_or("");
					device.name = xmlString(region, "DeviceName").value_or("iPhone");
					if (device.name.empty())
					{
						device.name = "iPhone";
					}
					found.push_back(device);
				}
				pos += needle.size();
			}
			return found;
		}

		std::vector<Device> parseDevices(const std::string& payload)
		{
			if (payload.size() >= 8 && payload.compare(0, 8, "bplist00") == 0)
			{
				BinaryPlist plist;
				if (!plist.parse(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()))
				{
					return {};
				}
				return plist.devices();
			}
			return xmlDevices(payload);
		}

		std::optional<int64_t> plistInteger(const std::string& payload, const std::string& key)
		{
			if (payload.size() >= 8 && payload.compare(0, 8, "bplist00") == 0)
			{
				BinaryPlist plist;
				if (!plist.parse(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()))
				{
					return std::nullopt;
				}
				return plist.findInt(key);
			}
			return xmlInteger(payload, key);
		}

		std::string plistMessage(const std::string& body)
		{
			return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
				"<plist version=\"1.0\"><dict>" + body + "</dict></plist>";
		}

		bool exchange(SOCKET socket, const std::string& xml, uint32_t tag, std::string& reply)
		{
			Header header{};
			header.length = static_cast<uint32_t>(sizeof(Header) + xml.size());
			header.version = 1;
			header.message = kPlistMessage;
			header.tag = tag;
			if (!sendAll(socket, reinterpret_cast<const char*>(&header), sizeof(header)) || !sendAll(socket, xml.data(), static_cast<int>(xml.size())))
			{
				return false;
			}
			Header response{};
			if (!recvAll(socket, reinterpret_cast<char*>(&response), sizeof(response)) || response.length < sizeof(Header))
			{
				return false;
			}
			const uint32_t payloadLength = response.length - static_cast<uint32_t>(sizeof(Header));
			reply.assign(payloadLength, '\0');
			return payloadLength == 0 || recvAll(socket, reply.data(), static_cast<int>(payloadLength));
		}

		SOCKET connectUsbmux()
		{
			SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (socket == INVALID_SOCKET)
			{
				return INVALID_SOCKET;
			}
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(kUsbmuxPort);
			inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
			if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
			{
				closesocket(socket);
				return INVALID_SOCKET;
			}
			setNoDelay(socket);
			return socket;
		}

		bool listFromService(std::vector<Device>& devices, std::string& failure)
		{
			SOCKET socket = connectUsbmux();
			if (socket == INVALID_SOCKET)
			{
				failure = "Apple Mobile Device Service is not running. Install Apple Devices or iTunes, then reconnect the cable.";
				return false;
			}
			const std::string list = plistMessage(
				"<key>MessageType</key><string>ListDevices</string>"
				"<key>ClientVersionString</key><string>usbmux_forward</string>"
				"<key>ProgName</key><string>usbmux_forward</string>");
			std::string reply;
			if (!exchange(socket, list, 1, reply))
			{
				closesocket(socket);
				failure = "Apple Mobile Device Service did not answer. Check the cable and that the service is running.";
				return false;
			}
			closesocket(socket);
			devices = parseDevices(reply);
			if (devices.empty())
			{
				failure = "No iPhone found. Use a data cable and tap Trust This Computer.";
				return false;
			}
			return true;
		}

		SOCKET openPhone(int phonePort, int preferredDeviceID, std::string& failure)
		{
			int deviceID = preferredDeviceID;
			if (deviceID == 0)
			{
				std::vector<Device> devices;
				if (!listFromService(devices, failure))
				{
					return INVALID_SOCKET;
				}
				deviceID = devices.front().id;
			}
			SOCKET socket = connectUsbmux();
			if (socket == INVALID_SOCKET)
			{
				failure = "Apple Mobile Device Service is not running. Install Apple Devices or iTunes, then reconnect the cable.";
				return INVALID_SOCKET;
			}
			std::string reply;
			const uint16_t networkPort = htons(static_cast<uint16_t>(phonePort));
			const std::string connectBody = plistMessage(
				"<key>MessageType</key><string>Connect</string>"
				"<key>ClientVersionString</key><string>usbmux_forward</string>"
				"<key>ProgName</key><string>usbmux_forward</string>"
				"<key>DeviceID</key><integer>" + std::to_string(deviceID) + "</integer>"
				"<key>PortNumber</key><integer>" + std::to_string(networkPort) + "</integer>");
			if (!exchange(socket, connectBody, 2, reply))
			{
				closesocket(socket);
				failure = "Could not open the phone port. Unlock the iPhone, keep VCamdroid in the foreground, and tap Trust This Computer.";
				return INVALID_SOCKET;
			}
			const std::optional<int64_t> result = plistInteger(reply, "Number");
			if (result && *result != 0)
			{
				closesocket(socket);
				failure = "The phone refused port " + std::to_string(phonePort)
					+ ". Unlock the iPhone, keep VCamdroid in the foreground, and tap Trust This Computer.";
				return INVALID_SOCKET;
			}
			return socket;
		}

		void splice(SOCKET client, SOCKET phone)
		{
			std::vector<char> buffer(16 * 1024);
			while (!gStop.load())
			{
				fd_set readSet;
				FD_ZERO(&readSet);
				FD_SET(client, &readSet);
				FD_SET(phone, &readSet);
				timeval wait{};
				wait.tv_sec = 1;
				const int ready = select(0, &readSet, nullptr, nullptr, &wait);
				if (ready < 0)
				{
					break;
				}
				auto pump = [&](SOCKET from, SOCKET to) -> bool
				{
					if (!FD_ISSET(from, &readSet))
					{
						return true;
					}
					const int read = recv(from, buffer.data(), static_cast<int>(buffer.size()), 0);
					if (read <= 0)
					{
						return false;
					}
					return sendAll(to, buffer.data(), read);
				};
				if (!pump(client, phone) || !pump(phone, client))
				{
					break;
				}
			}
			closesocket(client);
			closesocket(phone);
		}

		SOCKET listenLocal(int port)
		{
			SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (socket == INVALID_SOCKET)
			{
				return INVALID_SOCKET;
			}
			BOOL reuse = TRUE;
			setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(static_cast<uint16_t>(port));
			inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
			if (bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || listen(socket, 4) != 0)
			{
				closesocket(socket);
				return INVALID_SOCKET;
			}
			return socket;
		}
	}

	void requestStop()
	{
		gStop.store(true);
		const SOCKET previous = reinterpret_cast<SOCKET>(InterlockedExchangePointer(
			reinterpret_cast<PVOID*>(&gListen),
			reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(INVALID_SOCKET))));
		if (previous != INVALID_SOCKET)
		{
			closesocket(previous);
		}
	}

	void joinSplices()
	{
		std::lock_guard<std::mutex> guard(gThreadsMutex);
		for (std::thread& thread : gThreads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
		gThreads.clear();
	}

	void acceptLoop()
	{
		const int port = gPort;
		log(port, "Forwarding 127.0.0.1:" + std::to_string(port) + " to the phone.");
		while (!gStop.load())
		{
			const SOCKET client = accept(gListen, nullptr, nullptr);
			if (client == INVALID_SOCKET)
			{
				break;
			}
			setNoDelay(client);
			std::string failure;
			const SOCKET phone = openPhone(port, gDeviceID, failure);
			if (phone == INVALID_SOCKET)
			{
				log(port, failure);
				closesocket(client);
				continue;
			}
			std::lock_guard<std::mutex> guard(gThreadsMutex);
			gThreads.emplace_back(splice, client, phone);
		}
		joinSplices();
		if (gListen != INVALID_SOCKET)
		{
			const SOCKET previous = reinterpret_cast<SOCKET>(InterlockedExchangePointer(
				reinterpret_cast<PVOID*>(&gListen),
				reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(INVALID_SOCKET))));
			if (previous != INVALID_SOCKET)
			{
				closesocket(previous);
			}
		}
		log(port, "Port mapping removed.");
		gRunning.store(false);
	}

	void stop()
	{
		requestStop();
		if (gAcceptThread.joinable())
		{
			gAcceptThread.join();
		}
		joinSplices();
		if (gOwnsWSA)
		{
			WSACleanup();
			gOwnsWSA = false;
		}
		gRunning.store(false);
		gStop.store(false);
	}

	bool start(int port, int deviceId, std::string& error)
	{
		stop();
		gStop.store(false);
		gDeviceID = deviceId;
		gPort = port;
		WSADATA data{};
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
			error = "Winsock failed to start.";
			return false;
		}
		gOwnsWSA = true;
		gListen = listenLocal(port);
		if (gListen == INVALID_SOCKET)
		{
			error = "Could not listen on 127.0.0.1:" + std::to_string(port)
				+ ". Close usbmux_forward.exe if it is running, then try again.";
			WSACleanup();
			gOwnsWSA = false;
			return false;
		}
		gRunning.store(true);
		gAcceptThread = std::thread(acceptLoop);
		return true;
	}

	bool isRunning()
	{
		return gRunning.load();
	}

	int listeningPort()
	{
		return gPort;
	}

	ListResult listDevices()
	{
		ListResult result;
		WSADATA data{};
		const bool started = WSAStartup(MAKEWORD(2, 2), &data) == 0;
		if (!started)
		{
			result.error = "Winsock failed to start.";
			return result;
		}
		if (!listFromService(result.devices, result.error))
		{
			result.devices.clear();
		}
		else
		{
			result.error.clear();
		}
		WSACleanup();
		return result;
	}

	int run(int port)
	{
		gLog.store(true);
		std::string error;
		if (!start(port, 0, error))
		{
			log(port, error);
			gLog.store(false);
			return 1;
		}
		if (gAcceptThread.joinable())
		{
			gAcceptThread.join();
		}
		if (gOwnsWSA)
		{
			WSACleanup();
			gOwnsWSA = false;
		}
		gRunning.store(false);
		gLog.store(false);
		return 0;
	}
}
