#include "Plugin.h"

#include <functional>
#include <iostream>

#if defined _WIN32 || defined _WIN64
#include <Windows.h>

#define SO_EXT ".dll"
#else
#include <dlfcn.h>
#include <cstring>

#define SO_EXT ".so"
#endif


Plugin::Plugin(const std::string &id)
: m_id(id)
{
	clear();
}

void Plugin::clear() {
	m_handle = nullptr;
	internalSetOption = NULL;
	m_initialized = NULL;
	m_unloaded = NULL;
}


Plugin::~Plugin()
{
	unload();
}


bool Plugin::load(const std::string &path, std::shared_ptr<Settings> settings, std::shared_ptr<ComponentManager> manager, std::shared_ptr<NetworkManager> network)
{
	std::string file = path;
	if (!file.empty()) file.append("/");
	file = file + m_id + SO_EXT;

#if defined _WIN32 || defined _WIN64
	m_handle = (void*)LoadLibraryExA(file.c_str(), NULL, 0);
#else
	char *filepath = strdup(file.c_str());
	m_handle = dlopen(filepath, RTLD_NOW);
	free(filepath);
#endif

	if (!m_handle) return false;

	versionFunction getPluginApiVersion = (versionFunction)getFunction("pluginGetApiVersion");

	if (!getPluginApiVersion || getPluginApiVersion() != PLUGIN_API_VER) {
		std::cout << ">>> Unsupported plugin API version" << std::endl;
		return false;
	}

	initializeFunction initFn = (initializeFunction)getFunction("pluginLoading");
	internalSetOption = (setOptionFunction)getFunction("pluginSetOption");

	m_initialized = (initializedFunction)getFunction("pluginLoaded");
	m_unloaded = (unloadFunction)getFunction("pluginUnloaded");

	if (initFn) return initFn(settings, manager, network);

	return true;
}


void Plugin::initialize()
{
	if (m_initialized) m_initialized();
}


void Plugin::unload()
{
	if (m_handle) {
		if (m_unloaded) m_unloaded();

#if defined _WIN32 || defined _WIN64
		FreeLibrary((HMODULE)m_handle);
#else
		dlclose(m_handle);
#endif
	}

	clear();
}


void Plugin::setOption(const std::string &key, const std::string &value)
{
	if (internalSetOption) {
		internalSetOption(key.c_str(), value.c_str());
	}
}


void* Plugin::getFunction(const char *name)
{
#if defined _WIN32 || defined _WIN64
	return (void*)GetProcAddress((HMODULE)m_handle, name);
#else
	return dlsym(m_handle, name);
#endif
}
