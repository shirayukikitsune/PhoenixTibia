#include "Plugin.h"

#include "Settings.h"
#include "ComponentManager.h"
#include "NetworkManager.h"
#include "PluginComponent.h"

#include "Packet.h"

#include "../Plugin_Interserver/InterserverService.h"
#include "../Plugin_InterserverClient/InterserverClient.h"

#include "Account.h"
#include "Character.h"
#include "World.h"
#include "LoginService.h"

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

std::weak_ptr<Settings> g_settings;
std::weak_ptr<ComponentManager> g_manager;
std::weak_ptr<NetworkManager> g_network;
std::weak_ptr<InterserverClient> g_client;

LoggerComponent *g_logger;

std::shared_ptr<LoginService> g_service;

std::map<uint32_t, std::shared_ptr<World>> g_worlds;
RSA *g_rsa;

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

		g_service.reset(new LoginService);

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

	auto setting = g_settings.lock();
	if (!setting) return false;

	BIO *bio = BIO_new(BIO_s_file());
	if (bio == NULL || BIO_read_filename(bio, "data/rsa.pem") <= 0) {
		ERR_print_errors(bio);
		return false;
	}
	char *key = strdup(setting->getString("pem_key").c_str());
	g_rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, 0, key);
	free(key);

	if (!g_rsa)
		return false;

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
