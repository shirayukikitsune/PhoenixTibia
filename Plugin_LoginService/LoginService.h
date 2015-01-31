#pragma once

#include "NetworkService.h"

class LoginService :
	public NetworkService
{
public:
	LoginService();
	virtual ~LoginService();

	virtual std::string getName() const { return "login"; }

	virtual std::string getBindAddress();
	virtual unsigned short getBindPort();

	virtual bool needChecksum();

	virtual bool canHandle(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handleFirst(NetworkConnectionPtr connection, PacketPtr packet);

private:
	static void disconnectClient(NetworkConnectionPtr connection, uint8_t error, const std::string& message);

	static bool RSAdecrypt(NetworkConnectionPtr connection, PacketPtr packet);

	uint16_t requiredClientVersion;
	std::string requiredClientVersionString;
};

