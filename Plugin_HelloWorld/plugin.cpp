#include <iostream>
#include <string>
#include <memory>

#include "Settings.h"
#include "ComponentManager.h"
#include "Component.h"
#include "NetworkManager.h"
#include "PluginComponent.h"
#include "Plugin.h"

std::string name;
std::weak_ptr<Settings> g_settings;
std::weak_ptr<ComponentManager> g_manager;
std::weak_ptr<NetworkManager> g_network;
int loadedCallback;
int networkCallback;

PLUGIN_API_VER_FUNCTION

PLUGIN_ONLOADING
{
	g_settings = settings;
	g_manager = manager;
	g_network = network;
	name = "unnamed visitor";

	if (auto manager = g_manager.lock()) {
		loadedCallback = manager->OnServerReady.push([](){
			if (auto manager = g_manager.lock())
				std::cout << "[" << (manager->find("plugin")->asPlugin() ? "yay" : "nope") << "] Server ready already?" << std::endl;

			if (auto network = g_network.lock()) {
				boost::asio::deadline_timer timer(network->getIoService(), boost::posix_time::seconds(5));
				timer.async_wait([](const boost::system::error_code &ec) {
					if (!ec)
						std::cout << "Waited!" << std::endl;
					else std::cout << "Error: " << ec.message() << std::endl; // << this should happen, since the timer will get out of context and will be deleted
				});
			}
		});
	}

	return true;
}

PLUGIN_SETOPTION
{
	name = value;
}

PLUGIN_ONLOADED
{
	std::cout << "Hello, " << name << "!" << std::endl;
}

PLUGIN_ONUNLOADING
{
	if (auto manager = g_manager.lock())
		manager->OnServerReady -= loadedCallback;
}
