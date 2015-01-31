#include "AccountService.h"
#include "LoggerComponent.h"
#include "Settings.h"

extern std::weak_ptr<Settings> g_settings;
extern LoggerComponent *g_logger;

AccountService::AccountService()
{
}

AccountService::~AccountService()
{
}

std::string AccountService::getBindAddress()
{
	if (auto settings = g_settings.lock())
		return settings->getString("account_address");

	return "localhost";
}

unsigned short AccountService::getBindPort()
{
	if (auto settings = g_settings.lock())
		return (unsigned short)settings->getInteger("account_port");

	return 0U;
}

bool AccountService::needChecksum()
{
	return true;
}

bool AccountService::canHandle(NetworkConnectionPtr connection, PacketPtr packet)
{
	return packet->peek<uint8_t>() == 0x8F;
}

bool AccountService::handleFirst(NetworkConnectionPtr connection, PacketPtr packet)
{
	
}

bool AccountService::handle(NetworkConnectionPtr connection, PacketPtr packet)
{

}
