#pragma once

#include "NetworkDefinitions.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <list>

class ComponentManager;

class NetworkManager
{
public:
	NetworkManager(std::shared_ptr<ComponentManager> components);
	~NetworkManager();

	//! Adds a service to the managed list
	bool registerService(std::shared_ptr<NetworkService> service);
	//! Removes a service from the managed list
	bool unregisterService(std::shared_ptr<NetworkService> service);
	//! Removes a service from the managed list
	bool unregisterService(const std::string &name);
	//! Clears the managed list, for both services and listeners
	void clearServices();

	//! Starts all managed services
	void start();
	//! Stops all managed services
	void stop();
	//! Restarts all managed services
	void restart();

	//! Shorthand for NetworkManager::find
	std::shared_ptr<NetworkService> operator[](const std::string &name) { return this->find(name); }

	//! Returns a service, located by its name. nullptr if not found
	std::shared_ptr<NetworkService> find(const std::string &name);

	//! Returns the boost io service
	boost::asio::io_service& getIoService() { return m_ioService; }

private:
	bool m_running;
	boost::asio::io_service m_ioService;
	std::unordered_map<std::string, std::shared_ptr<NetworkService>> m_services;
	std::list<std::shared_ptr<NetworkListener>> m_listeners;
	std::shared_ptr<ComponentManager> m_components;
	std::shared_ptr<boost::asio::io_service::work> m_work;
};

