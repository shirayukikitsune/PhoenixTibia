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
	InterserverClient(boost::asio::io_service &ioService);
	~InterserverClient();

	void startConnect();
	void connect();
	virtual void close();

	phoenix::callback<false, void(void)> ConnectionSuccess, ConnectionFailed;

	void addCapability(const Capability &capability);
	void removeCapability(const Capability &capability);

	void requestNotify(const Capability &capability, std::function<void(bool)> callback);

private:
	std::string encipher(const std::string &what);

	void sendCapabilityList();

	void keepalive();
	void receive();

	std::map<Capability, phoenix::callback<false, void(bool)>> m_notifications;
	std::map<Capability, uint32_t> m_capabilities;
	boost::asio::deadline_timer m_keepalive;

protected:
	virtual bool needChecksum() {
		return true;
	}
};

