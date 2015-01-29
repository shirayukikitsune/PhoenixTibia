#include "Plugin.h"
#include "Component.h"
#include "ComponentManager.h"
#include "NetworkManager.h"
#include "Settings.h"
#include "InterserverService.h"

std::shared_ptr<InterserverService> g_service;

std::weak_ptr<Settings> g_settings;
LoggerComponent *g_logger;
std::weak_ptr<NetworkManager> g_network;
std::weak_ptr<ComponentManager> g_components;
int g_beforeNetworkStart;
int g_serverTerminating;

//! This function is always required and must always return PLUGIN_API_VER, so the kernel will know if it can run this plugin
//! (making sure that both kernel and plugin api versions matches)
PLUGIN_API_VER_FUNCTION

//! Function called when the plugin is loaded from the plugins.xml list
PLUGIN_ONLOADING
{
	// We will need these pointers later
	g_settings = settings;
	g_components = manager;
	g_network = network;

	if (auto manager = g_components.lock()) {
		auto component = manager->find("logger");
		if (component)
			g_logger = component->asLogger();

		g_service.reset(new InterserverService);

		// Even if network ready callback is called only once, we unregister it later to save a bit of memory, in case of the plugin is unloaded/reloaded
		g_beforeNetworkStart = manager->OnBeforeNetworkStart.push([]() {
			// Register our network service here, so the kernel will automatically start it
			if (auto network = g_network.lock()) {
				network->registerService(g_service);
			}
		});

		g_serverTerminating = manager->OnServerTerminating.push([]() {
			if (auto network = g_network.lock()) {
				network->unregisterService(g_service);
			}
		});
	}

	return true;
}

//! Function called when the plugin is about to be unloaded
PLUGIN_ONUNLOADING
{
	if (auto manager = g_components.lock()) {
		manager->OnBeforeNetworkStart -= g_beforeNetworkStart;
		manager->OnServerTerminating -= g_serverTerminating;
	}
}

PLUGIN_FUNCTION InterserverService* pluginGetServicePointer()
{
	return g_service.get();
}
