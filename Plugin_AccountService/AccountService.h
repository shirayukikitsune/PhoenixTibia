#pragma once

#include "NetworkService.h"

class AccountService
	: public NetworkService
{
public:
	AccountService();
	virtual ~AccountService();

	virtual std::string getName() const { return "account"; }

	virtual std::string getBindAddress();
	virtual unsigned short getBindPort();

	virtual bool needChecksum();

	virtual bool canHandle(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handleFirst(NetworkConnectionPtr connection, PacketPtr packet);

	virtual bool handle(NetworkConnectionPtr connection, PacketPtr packet);

private:

};
