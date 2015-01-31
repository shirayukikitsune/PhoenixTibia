#include "Plugin.h"

#include "Settings.h"
#include "ComponentManager.h"
#include "NetworkManager.h"
#include "PluginComponent.h"

#include "Packet.h"

#include "AccountService.h"
#include "../Plugin_InterserverClient/InterserverClient.h"

std::weak_ptr<Settings> g_settings;
std::weak_ptr<ComponentManager> g_manager;
std::weak_ptr<NetworkManager> g_network;
std::weak_ptr<InterserverClient> g_client;

std::shared_ptr<AccountService> g_service;

LoggerComponent *g_logger;

int g_readyCallback, g_connectedCallback, g_beforeNetworkStart, g_serverTerminating;

PLUGIN_API_VER_FUNCTION

PLUGIN_ONLOADING
{
	g_settings = settings;
	g_manager = manager;
	g_network = network;

	if (auto manager = g_manager.lock()) {
		auto component = manager->find("logger");
		if (component)
			g_logger = component->asLogger();

		g_readyCallback = manager->OnServerReady.push([]() {
			if (auto manager = g_manager.lock()) {
				std::weak_ptr<Plugin> _plugin = manager->find("plugin")->asPlugin()->getPlugin("interserver-client");
				if (auto plugin = _plugin.lock()) {
					auto getClient = (void(*)(std::weak_ptr<InterserverClient>&))(plugin->getFunction("pluginGetClientPointer"));
					getClient(g_client);

					if (auto client = g_client.lock()) {
						auto connectCallback = [] {
							if (auto client = g_client.lock())
								client->addCapability(g_service->getCapability());
						};

						if (client->socket().is_open())
							connectCallback();
						else
							g_connectedCallback = client->ConnectionSuccess.push(connectCallback);
					}
				}
			}
		});

		g_beforeNetworkStart = manager->OnBeforeNetworkStart.push([]() {
			if (auto network = g_network.lock())
				network->registerService(g_service);
		});

		g_serverTerminating = manager->OnServerTerminating.push([]() {
			if (auto network = g_network.lock())
				network->unregisterService(g_service);
		});
	}

	return true;
}

PLUGIN_ONUNLOADING
{
	if (auto client = g_client.lock()) {
		client->ConnectionSuccess -= g_connectedCallback;
	}
	if (auto manager = g_manager.lock()) {
		manager->OnServerReady -= g_readyCallback;
		manager->OnBeforeNetworkStart -= g_beforeNetworkStart;
		manager->OnServerTerminating -= g_serverTerminating;
	}
}
