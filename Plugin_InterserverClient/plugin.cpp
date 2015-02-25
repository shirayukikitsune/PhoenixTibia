#include "Plugin.h"

#include "Settings.h"
#include "ComponentManager.h"
#include "NetworkManager.h"
#include "ScriptComponent.h"

#include "InterserverClient.h"

std::weak_ptr<Settings> g_settings;
std::weak_ptr<ComponentManager> g_manager;
std::weak_ptr<NetworkManager> g_network;
std::shared_ptr<InterserverClient> g_client;
std::weak_ptr<Script> g_script;
int g_readyCallback = -1;

PLUGIN_API_VER_FUNCTION

PLUGIN_ONLOADING
{
	g_settings = settings;
	g_manager = manager;
	g_network = network;
	if (auto network = g_network.lock()) 
		g_client.reset(new InterserverClient(network->getIoService()));

	if (auto manager = g_manager.lock()) {
		g_readyCallback = manager->OnServerReady.push([]() {
			g_client->connect();
		});

		auto script = manager->find("script");
		if (script) {
			g_script = script->asScript()->getScript();
			g_client->initialize();
		}
	}

	return true;
}

PLUGIN_ONUNLOADING
{
	g_client->close();
	g_client.reset();
	if (g_readyCallback != -1) {
		if (auto manager = g_manager.lock())
			manager->OnServerReady.pop(g_readyCallback);
	}
}

PLUGIN_FUNCTION void pluginGetClientPointer(std::weak_ptr<InterserverClient> &client)
{
	client = g_client;
}
