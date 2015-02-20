#pragma once

#include "NetworkService.h"

#include <cstdint>
#include <map>
#include <functional>

class InterserverService
	: public NetworkService
{
public:
	typedef std::function<bool(NetworkConnectionPtr, PacketPtr)> PacketHandler;

	InterserverService();
	virtual ~InterserverService();

	virtual std::string getName() const { return "interserver"; }

	virtual std::string getBindAddress();
	virtual unsigned short getBindPort();

	virtual bool canHandle(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handleFirst(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handle(NetworkConnectionPtr connection, PacketPtr packet);

	virtual void removeConnection(NetworkConnectionPtr connection);

	void addHandler(uint16_t code, PacketHandler handler);
	void removeHandler(uint16_t code);

	enum {
		minProtocolVersion = 0x0001,
		maxProtocolVersion = 0x0001
	};

private:
	std::map<uint16_t, PacketHandler> m_handlers;
	std::multimap<std::string, Capability> m_capabilities;
	std::multimap<std::string, Capability> m_capabilityNotify;
	std::map<uint32_t, NetworkConnectionPtr> m_relay;

	std::string encipher(const std::string &what);
	void registerCapability(const Capability &capability);
};


