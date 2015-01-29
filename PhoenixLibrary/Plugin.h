#pragma once

#include <string>
#include <memory>

class ComponentManager;
class Settings;
class NetworkManager;

#define PLUGIN_API_VER 1

#if defined _WIN32 || defined _WIN64
#define PLUGIN_FUNCTION extern "C" __declspec(dllexport)
#else
#define PLUGIN_FUNCTION extern "C"
#endif

//! This macro should be used in every plugin, so the microkernel will test if the plugin is compatible with it
#define PLUGIN_API_VER_FUNCTION PLUGIN_FUNCTION int pluginGetApiVersion() \
{ \
	return PLUGIN_API_VER; \
}
//! This macro contains the signature of the "OnPluginLoading" callback, which is fired when the plugin has been asked to initialize
#define PLUGIN_ONLOADING PLUGIN_FUNCTION bool pluginLoading(std::weak_ptr<Settings> settings, std::weak_ptr<ComponentManager> manager, std::weak_ptr<NetworkManager> network)
//! This macro contains the signature of the "SetOption" callback, which is fired when the plugin loader finds a static option of the plugin
#define PLUGIN_SETOPTION PLUGIN_FUNCTION void pluginSetOption(const char *key, const char *value)
//! This macro contains the signature of the "OnPluginLoaded" callback, which is fired when all plugins have been loaded
#define PLUGIN_ONLOADED PLUGIN_FUNCTION void pluginLoaded()
//! This macro contains the signature of the "OnPluginUnloaded" callback, which is fired when this plugin have been unloaded
#define PLUGIN_ONUNLOADING PLUGIN_FUNCTION void pluginUnloaded()

class Plugin
{
public:
	Plugin(const std::string &id);
	~Plugin();
	void clear();

	bool load(const std::string &path, std::shared_ptr<Settings> settings, std::shared_ptr<ComponentManager> manager, std::shared_ptr<NetworkManager> network);
	void initialize();
	void unload();

	void setOption(const std::string &key, const std::string &value);

	const std::string& getId() const { return m_id; }

	void* getFunction(const char *name);

private:
	typedef void(*setOptionFunction)(const char *, const char *);
	typedef bool(*initializeFunction)(std::weak_ptr<Settings>, std::weak_ptr<ComponentManager>, std::weak_ptr<NetworkManager>);
	typedef void(*initializedFunction)(void);
	typedef void(*unloadFunction)(void);
	typedef int(*versionFunction)(void);

	setOptionFunction internalSetOption;
	initializedFunction m_initialized;
	unloadFunction m_unloaded;

	std::string m_id;
	void *m_handle;
};

