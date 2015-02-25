#pragma once

#include "Plugin.h"
#include "Callback.h"
#include "NetworkDefinitions.h"
#include "NetworkConnection.h"

#include <map>
#include <string>

#if defined _WIN32 || defined _WIN64
#ifdef PLUGIN_INTERSERVERCLIENT_EXPORTS
#define EXPORTS __declspec(dllexport)
#else
#define EXPORTS __declspec(dllimport)
#endif
#else
#define EXPORTS
#endif

class EXPORTS InterserverClient
	: public NetworkConnection
{
public:
	typedef phoenix::callback<false, void(bool)> notification_t;
	typedef std::function<void(PacketPtr)> requestpacket_t;
	typedef std::function<void(PacketPtr, PacketPtr)> requestpackethandler_t;

	InterserverClient(boost::asio::io_service &ioService);
	~InterserverClient();

	void initialize();

	void startConnect();
	void connect();
	virtual void close();

	phoenix::callback<false, void(void)> ConnectionSuccess, ConnectionFailed;

	void addCapability(const Capability &capability);
	void removeCapability(const Capability &capability);

	void requestNotify(const Capability &capability, notification_t::function_type &&callback);

	void requestPacketSerializable(const Capability &capability, const std::string& className, const PacketSerializable& data, requestpacket_t callback);
	void registerPacketSerializableHandler(const std::string& className, requestpackethandler_t handler);

	enum {
		protocolVersion = 0x0001
	};

private:
	std::string encipher(const std::string &what);

	void sendCapabilityList();

	void keepalive();
	void receive();

	std::map<Capability, notification_t> m_notifications;
	std::map<Capability, uint32_t> m_capabilities;
	boost::asio::deadline_timer m_keepalive;
	std::map<uint32_t, requestpacket_t> m_relays;
	std::map<std::string, requestpackethandler_t> m_handlers;

	enum class RelayOperation : uint8_t {
		RequestPacketSerializable = 1
	};

protected:
	virtual bool needChecksum() {
		return true;
	}
};

