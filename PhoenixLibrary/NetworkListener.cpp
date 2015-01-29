#include "NetworkListener.h"
#include "NetworkConnection.h"
#include "NetworkService.h"

#include "ComponentManager.h"

#include <iostream>
#include <algorithm>

using boost::asio::ip::tcp;

NetworkListener::NetworkListener(boost::asio::io_service& ioservice, std::shared_ptr<ComponentManager> components, tcp::endpoint endpoint)
: m_endpoint(endpoint), m_acceptor(ioservice)
{
	m_components = components;
}

NetworkListener::~NetworkListener()
{
}

void NetworkListener::start()
{
	if (!m_acceptor.is_open()) {
		try {
			m_acceptor.open(m_endpoint.protocol());
			m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));
			m_acceptor.bind(m_endpoint);
			m_acceptor.listen();

			startAccept();
		}
		catch (const boost::system::system_error &e) {
			std::cout << ">>> Failed to start listener at " << m_endpoint.address().to_string() << "@" << m_endpoint.port() << ": " << e.what() << std::endl;
		}
		catch (...) { }
	}
}

void NetworkListener::stop()
{
	if (m_acceptor.is_open()) {
		m_acceptor.cancel();
		m_acceptor.close();
	}
}

void NetworkListener::restart()
{
	stop();
	m_acceptor.get_io_service().reset();
	start();
}

unsigned short NetworkListener::getPort()
{
	return m_endpoint.port();
}

std::string NetworkListener::getAddress()
{
	return m_endpoint.address().to_string();
}

tcp::endpoint NetworkListener::getEndpoint()
{
	return m_endpoint;
}

NetworkListener::servicemap_t& NetworkListener::getServices()
{
	return m_services;
}

bool NetworkListener::registerService(std::shared_ptr<NetworkService> service)
{
	if (!service) return false;
	
	auto i = m_services.find(service->getName());

	if (i == m_services.end()) {
		m_services.emplace(service->getName(), service);
	}
	else {
		i->second = service;
	}

	return true;
}

bool NetworkListener::unregisterService(std::shared_ptr<NetworkService> service)
{
	if (!service) return false;

	return unregisterService(service->getName());
}

bool NetworkListener::unregisterService(const std::string &name)
{
	auto i = m_services.find(name);

	if (i == m_services.end()) return false;

	m_services.erase(i);

	return true;
}

void NetworkListener::clearServices()
{
	while (!m_services.empty()) {
		auto i = m_services.begin();
		m_services.erase(i);
	}
}

void NetworkListener::closeConnections()
{
	while (!m_connections.empty()) {
		(*m_connections.begin())->close();
		m_connections.erase(m_connections.begin());
	}
}

void NetworkListener::removeConnection(NetworkConnectionPtr connection)
{
	m_connections.erase(connection);
	if (connection->service()) connection->service()->removeConnection(connection);
	connection->close();
}

void NetworkListener::startAccept()
{
	NetworkConnectionPtr connection(new NetworkConnection(m_acceptor.get_io_service()));
	m_connections.emplace(connection);
	m_acceptor.async_accept(connection->socket(), std::bind(&NetworkListener::handleAccept, shared_from_this(), connection, std::placeholders::_1));
}

void NetworkListener::handleAccept(NetworkConnectionPtr connection, boost::system::error_code error)
{
	if (error != boost::asio::error::operation_aborted)
		startAccept();

	if (!error) {
		auto result = m_components->OnClientConnected(shared_from_this(), connection);

		// Check if any of the components "OnClientConnected" returned false and cancel the connection if that happened
		if (std::any_of(result.begin(), result.end(), [](const std::pair<int, bool> &value) { return !value.second; })) {
			connection->close();
			return;
		}

		connection->beginReading(std::bind(&NetworkListener::handleReceiveFirst, shared_from_this(), connection, std::placeholders::_1, std::placeholders::_2));
	}
}

void NetworkListener::handleReceiveFirst(NetworkConnectionPtr connection, PacketPtr packet, boost::system::error_code error)
{
	if (!error) {
		auto pos = packet->pos();

		for (auto &service : m_services) {
			if (service.second->needChecksum()) packet->skip(4); // go forward 4 bytes for the packet checksum

			if (service.second->canHandle(connection, packet)) {
				// The service can handle this, verify if it needs a checksum and validate it if needed
				if (service.second->needChecksum()) {
					uint32_t checksum = adlerChecksum(packet->data() + pos + 4, packet->size() - pos - 4);

					if (checksum != connection->getLastChecksum()) {
						// Invalid checksum, drop the connection
						removeConnection(connection);
						return;
					}
				}
				// skip the protocol type
				packet->skip(1);
				connection->service(service.second);

				// If the service handled successfully, then the connection must be kept alive, so exit this function
				if (service.second->handleFirst(connection, packet)) {
					connection->beginReading(std::bind(&NetworkListener::handleReceive, shared_from_this(), connection, std::placeholders::_1, std::placeholders::_2));
					return;
				}

				break;
			}
			else if (service.second->needChecksum()) packet->pos(pos); // Rewind the 4 bytes from the checksum
		}
	}

	// If we got here, then:
	// . There is no service that can handle this packet
	// . There is a service that can handle this packet, but the first failed to process the latter
	// So, we can do nothing with this connection and, therefore, must close it.
	removeConnection(connection);
}

void NetworkListener::handleReceive(NetworkConnectionPtr connection, PacketPtr packet, boost::system::error_code error)
{
	if (connection) {
		if (!error) {
			auto service = connection->service();

			if (service) {
				bool skipPacket = false;
				if (service->needChecksum()) {
					auto pos = packet->skip(4).pos();
					uint32_t checksum = adlerChecksum(packet->data() + pos, packet->size() - pos);

					if (checksum != connection->getLastChecksum()) // Invalid checksum, drop the packet but keep the connection alive
						skipPacket = true;
				}

				if (skipPacket || service->handle(connection, packet)) {
					// Schedule a new reading, since we were allowed to continue
					connection->beginReading(std::bind(&NetworkListener::handleReceive, shared_from_this(), connection, std::placeholders::_1, std::placeholders::_2));
					return;
				}
			}
		}
		
		removeConnection(connection);
	}
}
