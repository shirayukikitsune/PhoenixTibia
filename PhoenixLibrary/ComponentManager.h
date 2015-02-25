#pragma once

#include <algorithm>
#include <deque>
#include <memory>

#include "Callback.h"

class Component;
class Settings;
class NetworkManager;
class NetworkService;
class NetworkListener;
class NetworkConnection;

class ComponentManager
	: public std::enable_shared_from_this<ComponentManager>
{
public:
	ComponentManager();
	~ComponentManager();

	//! Adds all default components
	void registerDefaultComponents();
	/**
	 * @brief Adds a Component to the manager
	 */
	void registerComponent(std::shared_ptr<Component> component);

	//! Loads all managed components
	bool loadComponents(std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);

	//! Releases all managed components
	void unloadComponents();

	//! Called when the application is about to enter the main loop
	phoenix::callback<true, void(void)> OnServerReady;
	//! Called when the application is about to exit
	phoenix::callback<true, void(void)> OnServerTerminating;
	//! Called when the networking stack is loaded
	phoenix::callback<true, void(void)> OnBeforeNetworkStart;
	//! Called when a network service is about to start
	phoenix::callback<false, bool(std::shared_ptr<NetworkService>, std::shared_ptr<NetworkListener>)> OnNetworkServiceRegistering;
	//! Called when a network service has started
	phoenix::callback<false, void(std::shared_ptr<NetworkService>)> OnNetworkServiceRegistered;
	//! Called when a network service is about to be stopped
	phoenix::callback<false, bool(std::shared_ptr<NetworkService>)> OnNetworkServiceUnregistering;
	//! Called when a network service has stopped
	phoenix::callback<false, void(std::shared_ptr<NetworkService>)> OnNetworkServiceUnregistered;
	//! Called when a connection is received
	/**
	 * @remarks If any of the functions here returns false, the connection will be dropped
	 */
	phoenix::callback<false, bool(std::shared_ptr<NetworkListener>, std::shared_ptr<NetworkConnection>)> OnClientConnected;

	/**
	 * @brief Short-hand to ComponentManager::find
	 */
	std::shared_ptr<Component> operator[](const std::string &in) { return find(in); }

	/**
	 * @brief Finds a Component by its name
	 * @returns Component if name was registered, `nullptr` otherwise
	 */
	std::shared_ptr<Component> find(const std::string& name)
	{
		for (auto i = m_components.begin(); i != m_components.end(); ++i)
			if (i->first.compare(name) == 0)
				return i->second;

		return nullptr;
	}

private:
	std::deque<std::pair<std::string, std::shared_ptr<Component>>> m_components;
};

