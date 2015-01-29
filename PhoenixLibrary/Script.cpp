#include "Script.h"
#include "Settings.h"
#include "ComponentManager.h"
#include "NetworkManager.h"

#include <boost/filesystem.hpp>
#include <lua.hpp>

#include <cstring>
#include <iostream>
#include <deque>
#include <algorithm>

Script::Script()
{
	L = nullptr;
}


Script::~Script()
{
	deinitialize();
}

// These pointers should not be deleted, so we are safe to use them
Settings *settings;
ComponentManager *manager;
NetworkManager *network;

bool Script::initialize(const std::string &path, std::shared_ptr<Settings> settings, std::shared_ptr<ComponentManager> manager, std::shared_ptr<NetworkManager> network)
{
	namespace fs = boost::filesystem;
	if (!fs::exists(fs::path(path)) || !fs::is_directory(fs::path(path))) return false;

	::settings = settings.get();
	::manager = manager.get();
	::network = network.get();

	if (L) lua_close(L);

	L = luaL_newstate();
	if (!L) return false;

	luaL_openlibs(L);

	// Load common library folder
	std::deque<std::string> libs, specific;
	fs::path libDir(settings->getString("dataDirectory") + "/libs");
	fs::directory_iterator end;

	if (fs::exists(libDir) && fs::is_directory(libDir)) {
		for (fs::directory_iterator entry(libDir); entry != end; ++entry) {
			if (fs::is_regular_file(entry->path()))  {
				libs.emplace_back(entry->path().string());
			}
		}
	}

	// Sort the list, to make sure that files are loaded in a order that the user might want
	// NOTE: good practice: enumerate the files, like: 000-lib1.lua, 010-lib2.lua, 020-example.lua ...
	std::sort(libs.begin(), libs.end());

	// Now add the specific library path
	if (fs::exists(path + "/lib") && fs::is_directory(path + "/lib")) {
		for (fs::directory_iterator entry(path + "/lib"); entry != end; ++entry) {
			if (fs::is_regular_file(entry->path()))  {
				specific.emplace_back(entry->path().string());
			}
		}
	}
	std::sort(specific.begin(), specific.end());
	libs.insert(libs.end(), specific.begin(), specific.end());

	// Start loading
	for (auto &file : libs) {
		if (luaL_dofile(L, file.c_str())) {
			lua_close(L);
			L = nullptr;
			return false;
		}
	}

	registerFunctions();

	return true;
}

void Script::deinitialize()
{
	if (!L) return;

	while (!m_envs.empty()) {
		unload(m_envs.begin()->first);
	}

	lua_close(L);
	L = nullptr;
}

bool Script::load(const std::string &path, const std::string &scriptId)
{
	if (!L) return false;

	// Get our first stack position
	int start = lua_gettop(L);

	// Create the environment
	lua_newtable(L);

	// With this, we will have access to read the global environment
	// FIXME: should this be changed to a whitelist ?
	lua_newtable(L);
	lua_getglobal(L, "_G");
	lua_setfield(L, -2, "__index"); // Set the __index field on the metatable

	// Set the meta for the environment
	lua_setmetatable(L, -2);

	// Load the file as a function, it will be placed on top of the stack
	luaL_loadfile(L, path.c_str());

	// Copy the environment here
	lua_pushvalue(L, -2);

	// Set the environment for the pcall
	lua_setupvalue(L, start + 2, 1);

	// Call the function in our sandbox
	auto result = lua_pcall(L, 0, 0, 0);

	if (result != LUA_OK) {
		auto msg = lua_tostring(L, -1);
		lua_pop(L, lua_gettop(L) - start);
		std::cerr << ">> Script::load [" << scriptId << ":" << path << "] error: " << msg << std::endl;

		return false;
	}

	// Store our environment as a global on our current state
	lua_setglobal(L, scriptId.c_str());

	return true;
}

bool Script::unload(const std::string &scriptId)
{
	if (!prepareCall("scriptId", "onUnloaded") || !call(0, 0))
		return false;

	lua_getglobal(L, scriptId.c_str());

	auto check = lua_istable(L, -1);
	lua_pop(L, 1);
	if (!check) return false;

	lua_pushnil(L);
	lua_setglobal(L, scriptId.c_str());

	m_envs.erase(scriptId);

	return true;
}

bool Script::prepareCall(const std::string &scriptId, const std::string &function)
{
	// Check if the script is registered 
	lua_getglobal(L, scriptId.c_str());
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return false;
	}

	// Get the function
	lua_pushstring(L, function.c_str());
	lua_rawget(L, -2);
	// Check if it is a function
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2); // remove the "function" and the script environment
		return false;
	}

	// Remove the script global
	lua_insert(L, lua_absindex(L, -2));
	lua_pop(L, 1);

	return true;
}

bool Script::call(int nArgs, int nResults)
{
	return lua_pcall(L, nArgs, nResults, 0) == LUA_OK;
}

// **************************************
// Settings library
// **************************************
int settings_get(lua_State *L)
{
	const char* key = lua_tostring(L, 1);
	lua_pop(L, 1);

	lua_pushstring(L, settings->getString(key).c_str());
	return 1;
}

int settings_reload(lua_State *L)
{
	lua_pushboolean(L, settings->load());

	return 1;
}

const luaL_Reg settingsLib[] = {
	{ "get", settings_get },
	{ "reload", settings_reload },
	{ NULL, NULL }
};

int settings_register(lua_State *L)
{
	luaL_newlib(L, settingsLib);

	return 1;
}

// **************************************
// ComponentManager library
// **************************************
int manager_callbackRegister(lua_State *L)
{
	auto type = lua_tostring(L, 2);
	lua_pop(L, 1);
	int retval = -1;
	if (lua_isfunction(L, 1)) {
		// push manager table
		lua_getglobal(L, "manager");
		// try to get the function table
		lua_pushstring(L, type);
		lua_rawget(L, 2);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			// Create the key
			lua_newtable(L);
			lua_setfield(L, 2, type);
			// push the value to the stack
			lua_getfield(L, 2, type);
		}
		// add the function to the table
		int idx = luaL_len(L, 2) + 1;
		lua_pushvalue(L, 1);
		lua_rawseti(L, 3, idx);
		lua_pop(L, 3);

		auto getFunction = [](lua_State *L, int index, const char *type) {
			int start = lua_gettop(L);
			lua_getglobal(L, "manager");
			lua_pushstring(L, type);
			lua_rawget(L, -2);
			if (lua_istable(L, -1)) {
				lua_rawgeti(L, -1, index);
				lua_insert(L, start + 1);
			}
			lua_pop(L, lua_gettop(L) - start - 1);
		};

		if (stricmp("OnServerReady", type) == 0) {
			auto callback = [L, idx, type, getFunction]() {
				getFunction(L, idx, type);
				lua_pcall(L, 0, 0, 0);
			};

			retval = manager->OnServerReady.push(callback);
		}
		else if (stricmp("OnBeforeNetworkStart", type) == 0) {
			auto callback = [L, idx, type, getFunction]() {
				getFunction(L, idx, type);
				lua_pcall(L, 0, 0, 0);
			};

			retval = manager->OnBeforeNetworkStart.push(callback);
		}
	}
	else lua_pop(L, 1);

	if (retval == -1) lua_pushnil(L);
	else lua_pushinteger(L, retval);

	return 1;
}

int manager_callbackUnregister(lua_State *L)
{
	int callback = lua_tointeger(L, 1);
	auto type = lua_tostring(L, 2);
	lua_pop(L, 2);

	if (stricmp("OnServerReady", type) == 0) {
		manager->OnServerReady.pop(callback);
	}
	if (stricmp("OnBeforeNetworkStart", type) == 0) {
		manager->OnBeforeNetworkStart.pop(callback);
	}
	
	return 0;
}

const luaL_Reg managerLib[] = {
	{ "register", manager_callbackRegister },
	{ "unregister", manager_callbackUnregister },
	{ NULL, NULL },
};

int manager_register(lua_State *L)
{
	luaL_newlib(L, managerLib);
	return 1;
}

// **************************************
// Network library
// **************************************

int network_stop(lua_State *L)
{
	network->stop();

	return 0;
}

const luaL_Reg networkLib[] = {
	{ "stop", network_stop },
	{ NULL, NULL }
};

int network_register(lua_State *L)
{
	luaL_newlib(L, networkLib);
	return 1;
}

void Script::registerFunctions()
{
	luaL_requiref(L, "settings", settings_register, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "manager", manager_register, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "network", network_register, 1);
	lua_pop(L, 1);
}
