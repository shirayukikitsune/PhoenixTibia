#pragma once

#include "NetworkDefinitions.h"

#include <unordered_map>
#include <memory>
#include <set>

class ComponentManager;

class NetworkListener
	: public std::enable_shared_from_this<NetworkListener>
{
public:
	typedef std::unordered_map<std::string, std::shared_ptr<NetworkService>> servicemap_t;

	NetworkListener(boost::asio::io_service& ioservice, std::shared_ptr<ComponentManager> components, boost::asio::ip::tcp::endpoint endpoint);
	~NetworkListener();

	void start();
	void stop();
	void restart();

	//! Returns the port which this listener is bound to
	unsigned short getPort();
	//! Returns the address which this listener is bound to
	std::string getAddress();
	//! Returns the entire endpoint which this listener is bound to
	boost::asio::ip::tcp::endpoint getEndpoint();

	//! Returns the services running in this listener
	servicemap_t& getServices();

	//! Adds a service to the managed list
	bool registerService(std::shared_ptr<NetworkService> service);
	//! Removes a service from the managed list
	bool unregisterService(std::shared_ptr<NetworkService> service);
	//! Removes a service from the managed list
	bool unregisterService(const std::string &name);
	//! Removes all services from the managed list
	void clearServices();
	//! Closes all opened connections
	void closeConnections();

private:
	boost::asio::ip::tcp::endpoint m_endpoint;
	boost::asio::ip::tcp::acceptor m_acceptor;
	servicemap_t m_services;
	std::shared_ptr<ComponentManager> m_components;
	std::set<NetworkConnectionPtr> m_connections;

	void removeConnection(NetworkConnectionPtr connection);

	void startAccept();

	void handleAccept(NetworkConnectionPtr connection, boost::system::error_code error);
	void handleReceiveFirst(NetworkConnectionPtr connection, PacketPtr packet, boost::system::error_code error);
	void handleReceive(NetworkConnectionPtr connection, PacketPtr packet, boost::system::error_code error);
};
