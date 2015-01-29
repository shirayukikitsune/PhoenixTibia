#include "NetworkManager.h"
#include "NetworkService.h"
#include "NetworkConnection.h"
#include "NetworkListener.h"

#include "ComponentManager.h"

#include <iostream>
#include <algorithm>

using boost::asio::ip::tcp;

NetworkManager::NetworkManager(std::shared_ptr<ComponentManager> components)
{
	m_components = components;
	m_running = false;
	m_work.reset(new boost::asio::io_service::work(m_ioService));
}


NetworkManager::~NetworkManager()
{
}


bool NetworkManager::registerService(std::shared_ptr<NetworkService> service)
{
	if (!service) return false;

	std::cout << ">> Registering network service " << service->getName() << std::endl;

	try {
		auto i = m_services.find(service->getName());
		auto listener = m_listeners.begin();
		// Find a listener for this service
		while (listener != m_listeners.end()) {
			if ((*listener)->getPort() == service->getBindPort() && (*listener)->getAddress().compare(service->getBindAddress()) == 0) break;
			++listener;
		}

		// If not found a listener, then create a new one
		if (listener == m_listeners.end()) {
			m_listeners.emplace_front(new NetworkListener(m_ioService, m_components, tcp::endpoint(boost::asio::ip::address::from_string(service->getBindAddress()), service->getBindPort())));
			listener = m_listeners.begin();

			if (m_running) (*listener)->start();
		}

		auto result = m_components->OnNetworkServiceRegistering(service, *listener);
		if (std::any_of(result.begin(), result.end(), [](const std::pair<int, bool> &r){ return !r.second; }))
			return false;

		if (i != m_services.end()) {
			// Service with this ID already registered, unload it
			i->second->getListener()->unregisterService(i->second);

			i->second = service;
		}
		else {
			// New service
			m_services.emplace(service->getName(), service);
		}

		service->setListener(*listener);
		(*listener)->registerService(service);

		m_components->OnNetworkServiceRegistered(service);
			
		return true;
	}
	catch (const boost::system::system_error &e) {
		std::cout << ">>> Error registering " << service->getName() << ": " << e.what() << std::endl;
	}

	return false;
}


bool NetworkManager::unregisterService(std::shared_ptr<NetworkService> service)
{
	if (!service) return false;

	return unregisterService(service->getName());
}


bool NetworkManager::unregisterService(const std::string &name)
{
	auto i = m_services.find(name);

	if (i != m_services.end()) {
		std::cout << ">>> Unregistering network service " << name << std::endl;

		auto listener = i->second->getListener();

		listener->unregisterService(i->second);

		m_services.erase(i);

		// Remove the listener if no more services to serve
		if (listener->getServices().empty()) {
			auto i = m_listeners.begin();
			while (i != m_listeners.end()) {
				if ((*i)->getPort() == listener->getPort() && (*i)->getAddress().compare(listener->getAddress()) == 0) {
					m_listeners.erase(i);
					listener->stop();
					listener.reset();
					break;
				}
			}
		}

		return true;
	}

	return false;
}


void NetworkManager::clearServices()
{
	while (!m_services.empty())
		unregisterService(m_services.begin()->first);
}


void NetworkManager::start()
{
	m_running = true;
	
	m_ioService.reset();

	std::cout << ">> Bound addresses: " << std::endl;

	for (auto &i : m_listeners) {
		std::cout << ">>> ";
		if (i->getEndpoint().address().is_v6())
			std::cout << "[" << i->getAddress() << "]";
		else std::cout << i->getAddress();
		std::cout << ":" << i->getPort() << ":";
		for (auto &service : i->getServices()) {
			std::cout << "\t" << service.second->getName();
		}
		std::cout << std::endl;
		i->start();
	}
}


void NetworkManager::stop()
{
	m_running = false;

	for (auto &i : m_listeners) {
		i->closeConnections();
		i->stop();
	}
	
	m_work.reset();
}


void NetworkManager::restart()
{
	stop();

	start();
}


std::shared_ptr<NetworkService> NetworkManager::find(const std::string &name)
{
	auto i = m_services.find(name);

	if (i == m_services.end()) return nullptr;

	return i->second;
}
