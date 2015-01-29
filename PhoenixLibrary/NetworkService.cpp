#include "NetworkService.h"
#include "NetworkConnection.h"

using boost::asio::ip::tcp;

NetworkService::NetworkService()
{
}


NetworkService::~NetworkService()
{
}


std::string NetworkService::getBindAddress()
{
	return "0.0.0.0";
}

unsigned short NetworkService::getBindPort()
{
	return 0;
}

Capability NetworkService::getCapability()
{
	return Capability(getName(), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(getBindAddress()), getBindPort()), std::shared_ptr<NetworkConnection>());
}

void NetworkService::setListener(std::shared_ptr<NetworkListener> listener)
{
	m_listener = listener;
}

std::shared_ptr<NetworkListener> NetworkService::getListener()
{
	return m_listener;
}

bool NetworkService::needChecksum()
{
	return true;
}

bool NetworkService::canHandle(NetworkConnectionPtr connection, PacketPtr packet)
{
	return false;
}

bool NetworkService::handleFirst(NetworkConnectionPtr connection, PacketPtr packet)
{
	return true;
}

bool NetworkService::handle(NetworkConnectionPtr connection, PacketPtr packet)
{
	return true;
}

void NetworkService::removeConnection(NetworkConnectionPtr connection)
{

}
