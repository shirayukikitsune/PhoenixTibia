#include "Script.h"

#include "ComponentManager.h"
#include "LuaNetworkService.h"
#include "NetworkConnection.h"
#include "NetworkManager.h"
#include "Settings.h"

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
			std::cerr << lua_tostring(L, -1) << std::endl;
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
// LUA auxiliary functions
// **************************************
int createMetatable(lua_State *L, const char *tableName, const luaL_Reg *functions)
{
	luaL_newmetatable(L, tableName);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	luaL_setfuncs(L, functions, 0);

	return 1;
}

LuaNetworkService* checkNetworkService(lua_State *L) {
	auto udata = luaL_checkudata(L, 1, "PhoenixTibia.NetworkService");
	luaL_argcheck(L, udata != NULL, 1, "`networkservice' expected");
	return (LuaNetworkService*)udata;
}

Packet* lua_topacket(lua_State *L, int index) {
	auto udata = luaL_checkudata(L, index, "PhoenixTibia.Packet");
	luaL_argcheck(L, udata != NULL, index, "`packet' expected");
	return (Packet*)udata;
}

PacketSerializable* lua_topacketserializable(lua_State *L, int index) {
	auto udata = luaL_checkudata(L, index, "PhoenixTibia.PacketSerializable");
	luaL_argcheck(L, udata != NULL, index, "`packetserializable' expected");
	return (PacketSerializable*)udata;
}

NetworkConnection* checkNetworkConnection(lua_State *L) {
	auto udata = luaL_checkudata(L, 1, "PhoenixTibia.Connection");
	luaL_argcheck(L, udata != NULL, 1, "`connection' expected");
	return (NetworkConnection*)udata;
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
		int idx = (int)luaL_len(L, 2) + 1;
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

		auto callback = [L, idx, type, getFunction]() {
			getFunction(L, idx, type);
			int call = lua_pcall(L, 0, 0, 0);

			if (call != LUA_OK) {
				std::cerr << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		};

		auto callbackRegistering = [L, idx, type, getFunction](std::shared_ptr<NetworkService> service, std::shared_ptr<NetworkListener> listener) {
			getFunction(L, idx, type);
			// TODO: push a lua_userobject for both service and listener
			lua_pushnil(L);
			lua_pushnil(L);

			int call = lua_pcall(L, 2, 1, 0);

			bool result = false;
			if (call == LUA_OK) {
				result = lua_toboolean(L, -1) != 0;
			}
			else {
				std::cerr << lua_tostring(L, -1) << std::endl;
			}

			lua_pop(L, 1);

			return result;
		};

		auto callbackRegistered = [L, idx, type, getFunction](std::shared_ptr<NetworkService> service) {
			getFunction(L, idx, type);
			// TODO: push a lua_userobject for service
			lua_pushnil(L);

			int call = lua_pcall(L, 1, 0, 0);

			if (call != LUA_OK) {
				std::cerr << lua_tostring(L, -1) << std::endl;
				lua_pop(L, 1);
			}
		};

		auto callbackUnregistering = [L, idx, type, getFunction](std::shared_ptr<NetworkService> service) {
			getFunction(L, idx, type);
			// TODO: push a lua_userobject for service
			lua_pushnil(L);

			int call = lua_pcall(L, 1, 1, 0);

			bool result = false;
			if (call == LUA_OK) {
				result = lua_toboolean(L, -1) != 0;
			}
			else {
				std::cerr << lua_tostring(L, -1) << std::endl;
			}

			lua_pop(L, 1);
			return result;
		};

		auto callbackClientConnected = [L, idx, type, getFunction](std::shared_ptr<NetworkListener> listener, NetworkConnectionPtr connection) {
			getFunction(L, idx, type);
			// TODO: push a lua_userobject for listener
			lua_pushnil(L);
			lua_pushconnection(L, connection.get());

			int call = lua_pcall(L, 2, 1, 0);

			bool result = false;
			if (call == LUA_OK) {
				result = lua_toboolean(L, -1) != 0;
			}
			else {
				std::cerr << lua_tostring(L, -1) << std::endl;
			}

			lua_pop(L, 1);
			return result;
		};

		if (stricmp("OnServerReady", type) == 0) {
			retval = manager->OnServerReady.push(callback);
		}
		else if (stricmp("OnServerTerminating", type) == 0) {
			retval = manager->OnServerTerminating.push(callback);
		}
		else if (stricmp("OnBeforeNetworkStart", type) == 0) {
			retval = manager->OnBeforeNetworkStart.push(callback);
		}
		else if (stricmp("OnNetworkServiceRegistering", type) == 0) {
			retval = manager->OnNetworkServiceRegistering.push(callbackRegistering);
		}
		else if (stricmp("OnNetworkServiceRegistered", type) == 0) {
			retval = manager->OnNetworkServiceRegistered.push(callbackRegistered);
		}
		else if (stricmp("OnNetworkServiceUnregistering", type) == 0) {
			retval = manager->OnNetworkServiceUnregistering.push(callbackUnregistering);
		}
		else if (stricmp("OnNetworkServiceUnregistered", type) == 0) {
			retval = manager->OnNetworkServiceUnregistered.push(callbackRegistered);
		}
		else if (stricmp("OnClientConnected", type) == 0) {
			retval = manager->OnClientConnected.push(callbackClientConnected);
		}
	}
	else lua_pop(L, 1);

	if (retval == -1) lua_pushnil(L);
	else lua_pushinteger(L, retval);

	return 1;
}

int manager_callbackUnregister(lua_State *L)
{
	int callback = (int)lua_tointeger(L, 1);
	auto type = lua_tostring(L, 2);
	lua_pop(L, 2);

	if (stricmp("OnServerReady", type) == 0) {
		manager->OnServerReady.pop(callback);
	}
	else if (stricmp("OnServerTerminating", type) == 0) {
		manager->OnServerTerminating.pop(callback);
	}
	else if (stricmp("OnBeforeNetworkStart", type) == 0) {
		manager->OnBeforeNetworkStart.pop(callback);
	}
	else if (stricmp("OnNetworkServiceRegistering", type) == 0) {
		manager->OnNetworkServiceRegistering.pop(callback);
	}
	else if (stricmp("OnNetworkServiceRegistered", type) == 0) {
		manager->OnNetworkServiceRegistered.pop(callback);
	}
	else if (stricmp("OnNetworkServiceUnregistering", type) == 0) {
		manager->OnNetworkServiceUnregistering.pop(callback);
	}
	else if (stricmp("OnNetworkServiceUnregistered", type) == 0) {
		manager->OnNetworkServiceUnregistered.pop(callback);
	}
	else if (stricmp("OnClientConnected", type) == 0) {
		manager->OnClientConnected.pop(callback);
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

void copyFunction(lua_State *fromState, lua_State *toState) {
	struct _ud {
		char *function;
		size_t size;
	} data;
	data.function = nullptr;
	data.size = 0;

	lua_Writer writer = [](lua_State *L, const void* p, size_t sz, void* ud) {
		auto data = (_ud*)ud;
		data->function = (char*)realloc(data->function, data->size + sz);
		memcpy(data->function + data->size, p, sz);
		data->size += sz;
		return 0;
	};
	lua_dump(fromState, writer, &data, 0);
	luaL_loadbuffer(toState, data.function, data.size, NULL);
}

int networkservice_tostring(lua_State *L) {
	auto service = checkNetworkService(L);

	lua_pushfstring(L, "NetworkService('%s')", service->getName().c_str());

	return 1;
}

int networkservice_setprop(lua_State *L) {
	auto service = checkNetworkService(L);
	auto prop = luaL_checkstring(L, 2);

	if (stricmp(prop, "address") == 0) {
		auto address = luaL_checkstring(L, 3);
		service->bindAddress = address;
	}
	else if (stricmp(prop, "port") == 0) {
		auto port = luaL_checkinteger(L, 3);
		service->bindPort = (uint16_t)port;
	}
	else if (stricmp(prop, "needChecksum") == 0) {
		service->checksum = lua_toboolean(L, 3) != 0;
	}
	else if (stricmp(prop, "canHandle") == 0 || stricmp(prop, "handleFirst") == 0 || stricmp(prop, "handle") == 0 || stricmp(prop, "removeConnection") == 0) {
		luaL_checktype(L, 3, LUA_TFUNCTION);

		copyFunction(L, service->getState());

		lua_setglobal(service->getState(), prop);
	}
	else luaL_error(L, "invalid property for `networkservice': %s", prop);

	return 0;
}

int networkservice_getprop(lua_State *L) {
	auto service = checkNetworkService(L);
	auto prop = luaL_checkstring(L, 2);

	if (stricmp(prop, "address") == 0) {
		lua_pushstring(L, service->getBindAddress().c_str());
	}
	else if (stricmp(prop, "port") == 0) {
		lua_pushinteger(L, service->getBindPort());
	}
	else if (stricmp(prop, "needChecksum") == 0) {
		lua_pushboolean(L, service->needChecksum());
	}
	else if (stricmp(prop, "name") == 0) {
		lua_pushstring(L, service->getName().c_str());
	}
	else lua_pushnil(L);

	return 1;
}

int networkservice_getname(lua_State *L) {
	auto service = checkNetworkService(L);

	lua_pushstring(L, service->getName().c_str());

	return 1;
}

int networkservice_setbindings(lua_State *L) {
	auto service = checkNetworkService(L);
	auto address = luaL_checkstring(L, 2);
	auto port = (unsigned short)luaL_checkinteger(L, 3);

	service->bindAddress = address;
	service->bindPort = port;

	return 0;
}

int networkservice_unregister(lua_State *L) {
	auto service = checkNetworkService(L);

	lua_pushboolean(L, network->unregisterService(service->shared_from_this()));

	return 1;
}

int networkservice_gc(lua_State *L) {
	auto service = checkNetworkService(L);

	service->~LuaNetworkService();

	return 0;
}

const luaL_Reg networkServiceFunctions[] = {
	{ "__gc", networkservice_gc },
	{ "__tostring", networkservice_tostring },
	{ "__newindex", networkservice_setprop },
	{ "__index", networkservice_getprop },
	{ "name", networkservice_getname },
	{ "bind", networkservice_setbindings },
	{ "unregister", networkservice_unregister },
	{ NULL, NULL }
};

int networkservice_new(lua_State *L) {
	auto name = luaL_checkstring(L, 1);

	auto service = new (lua_newuserdata(L, sizeof LuaNetworkService)) LuaNetworkService;
	service->initialize(L, name);

	luaL_getmetatable(L, "PhoenixTibia.NetworkService");
	lua_setmetatable(L, -2);

	return 1;
}

const luaL_Reg networkServiceLib[] = {
	{ "new", networkservice_new },
	{ NULL, NULL }
};

int networkservice_registerLib(lua_State *L)
{
	createMetatable(L, "PhoenixTibia.NetworkService", networkServiceFunctions);

	luaL_newlib(L, networkServiceLib);

	return 1;
}

int packet_getprop(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	auto prop = luaL_checkstring(L, 2);
	lua_pop(L, 2);

	if (stricmp(prop, "size") == 0) {
		lua_pushinteger(L, packet->size());
	}
	else if (stricmp(prop, "position") == 0) {
		lua_pushinteger(L, packet->pos());
	}
	else if (stricmp(prop, "start") == 0) {
		lua_pushinteger(L, packet->start());
	}
	else lua_pushnil(L);

	return 1;
}

int packet_setprop(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	auto prop = luaL_checkstring(L, 2);

	if (stricmp(prop, "size") == 0) {
		auto size = luaL_checkinteger(L, 3);
		packet->size(size);
	}
	else if (stricmp(prop, "position") == 0) {
		auto pos = luaL_checkinteger(L, 3);
		packet->pos(pos);
	}

	lua_pop(L, 3);

	return 0;
}

int packet_gc(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	packet->~Packet();

	return 0;
}

int packet_reset(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	packet->reset();

	return 0;
}

template <typename T>
int packet_peekInteger(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	lua_pushinteger(L, packet->peek<T>());

	return 1;
}

int packet_peekString(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	lua_pushstring(L, packet->peek<std::string>().c_str());

	return 1;
}

template <typename T>
int packet_popInteger(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	lua_pushinteger(L, packet->pop<T>());

	return 1;
}

int packet_popString(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	lua_pop(L, 1);

	lua_pushstring(L, packet->pop<std::string>().c_str());

	return 1;
}

template <typename T>
int packet_pushInteger(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	auto value = luaL_checkinteger(L, 2);
	lua_pop(L, 2);

	packet->push<T>(value);

	return 0;
}

int packet_pushString(lua_State *L) {
	auto packet = lua_topacket(L, 1);
	auto value = luaL_checkstring(L, 2);
	lua_pop(L, 2);

	packet->push<std::string>(value);

	return 0;
}

const luaL_Reg packetFunctions[] = {
	//{ "__index", packet_getprop },
	//{ "__newindex", packet_setprop },
	{ "__gc", packet_gc },

	{ "reset", packet_reset },

	{ "peekU8", packet_peekInteger<uint8_t> },
	{ "peekU16", packet_peekInteger<uint16_t> },
	{ "peekU32", packet_peekInteger<uint32_t> },
	{ "peekS8", packet_peekInteger<int8_t> },
	{ "peekS16", packet_peekInteger<int16_t> },
	{ "peekS32", packet_peekInteger<int32_t> },
	{ "peekString", packet_peekString },

	{ "popU8", packet_popInteger<uint8_t> },
	{ "popU16", packet_popInteger<uint16_t> },
	{ "popU32", packet_popInteger<uint32_t> },
	{ "popS8", packet_popInteger<int8_t> },
	{ "popS16", packet_popInteger<int16_t> },
	{ "popS32", packet_popInteger<int32_t> },
	{ "popString", packet_popString },

	{ "pushU8", packet_pushInteger<uint8_t> },
	{ "pushU16", packet_pushInteger<uint16_t> },
	{ "pushU32", packet_pushInteger<uint32_t> },
	{ "pushS8", packet_pushInteger<int8_t> },
	{ "pushS16", packet_pushInteger<int16_t> },
	{ "pushS32", packet_pushInteger<int32_t> },
	{ "pushString", packet_pushString },

	{ NULL, NULL }
};

int packet_new(lua_State *L) {
	auto packet = new (lua_newuserdata(L, sizeof Packet)) Packet;

	luaL_getmetatable(L, "PhoenixTibia.Packet");
	lua_setmetatable(L, -2);

	return 1;
}

int lua_pushpacket(lua_State *L, Packet* packet)
{
	lua_pushlightuserdata(L, packet);

	luaL_getmetatable(L, "PhoenixTibia.Packet");
	lua_setmetatable(L, -2);

	return 1;
}

const luaL_Reg packetLib[] = {
	{ "new", packet_new },
	{ NULL, NULL }
};

int packet_registerLib(lua_State *L)
{
	createMetatable(L, "PhoenixTibia.Packet", packetFunctions);

	luaL_newlib(L, packetLib);

	return 1;
}

int packetserializable_gc(lua_State *L)
{
	PacketSerializable* data = lua_topacketserializable(L, 1);

	if (!lua_islightuserdata(L, 1)) {
		data->~PacketSerializable();
	}

	return 0;
}

int packetserializable_write(lua_State *L)
{
	PacketSerializable* data = lua_topacketserializable(L, 1);
	Packet* packet = lua_topacket(L, 2);

	data->write(*packet);

	return 0;
}

int packetserializable_read(lua_State *L)
{
	PacketSerializable* data = lua_topacketserializable(L, 1);
	Packet* packet = lua_topacket(L, 2);

	data->read(*packet);

	return 0;
}

const luaL_Reg packetserializableFunctions[] = {
	{ "__gc", packetserializable_gc },

	{ "write", packetserializable_write },
	{ "read", packetserializable_read },
	{ NULL, NULL }
};

int packetserializable_new(lua_State *L)
{
	LuaPacketSerializable* packet = new (lua_newuserdata(L, sizeof(LuaPacketSerializable))) LuaPacketSerializable;
	packet->initialize(L);

	luaL_getmetatable(L, "PhoenixTibia.PacketSerializable");
	lua_setmetatable(L, -2);

	return 1;
}

int lua_pushpacketserializable(lua_State *L, PacketSerializable* data)
{
	lua_pushlightuserdata(L, data);

	luaL_getmetatable(L, "PhoenixTibia.PacketSerializable");
	lua_setmetatable(L, -2);

	return 1;
}

const luaL_Reg packetserializableLib[] =
{
	{ "new", packetserializable_new },
	{ NULL, NULL }
};

int packetserializable_registerLib(lua_State *L)
{
	createMetatable(L, "PhoenixTibia.PacketSerializable", packetserializableFunctions);

	luaL_newlib(L, packetserializableLib);

	return 1;
}

int connection_gc(lua_State *L)
{
	auto connection = checkNetworkConnection(L);
	lua_pop(L, 1);

	if (connection->isLua())
		connection->~NetworkConnection();

	return 0;
}

int connection_close(lua_State *L)
{
	auto connection = checkNetworkConnection(L);

	connection->close();

	return 0;
}

int connection_send(lua_State *L)
{
	auto connection = checkNetworkConnection(L);
	auto packet = std::shared_ptr<Packet>(new Packet((Packet*)lua_touserdata(L, 2)));
	
	connection->send(packet);

	return 0;
}

int connection_getLastChecksum(lua_State *L)
{
	auto connection = checkNetworkConnection(L);
	lua_pop(L, 1);

	lua_pushinteger(L, connection->getLastChecksum());

	return 1;
}

int connection_setKeys(lua_State *L)
{
	auto connection = checkNetworkConnection(L);
	std::array<uint32_t, 4> keys;

	if (lua_gettop(L) >= 5) {
		for (int i = 0; i < 4; ++i) {
			keys[i] = (uint32_t)lua_tointeger(L, 2 + i);
		}
		lua_pop(L, 5);
	}
	else {
		luaL_checktype(L, 2, LUA_TTABLE);
		for (int i = 0; i < 4; ++i) {
			lua_pushinteger(L, i + 1);
			lua_gettable(L, 2);
			keys[i] = (uint32_t)lua_tointeger(L, -1);
			lua_pop(L, 1);
		}
		lua_pop(L, 2);
	}

	connection->setKeys(keys);

	return 0;
}

const luaL_Reg connectionFunctions[] = {
	{ "__gc", connection_gc },
	{ "close", connection_close },
	{ "send", connection_send },
	{ "getLastChecksum", connection_getLastChecksum },
	{ "setKeys", connection_setKeys },
	{ NULL, NULL }
};

int connection_new(lua_State *L) 
{
	auto connection = new (lua_newuserdata(L, sizeof NetworkConnection)) NetworkConnection(network->getIoService(), true);

	luaL_getmetatable(L, "PhoenixTibia.Connection");
	lua_setmetatable(L, -2);

	return 1;
}

int lua_pushconnection(lua_State *L, NetworkConnection* connection)
{
	lua_pushlightuserdata(L, connection);

	luaL_getmetatable(L, "PhoenixTibia.Connection");
	lua_setmetatable(L, -2);

	return 1;
}

const luaL_Reg connectionLib[] = {
	{ "new", connection_new },
	{ NULL, NULL }
};

int connection_registerLib(lua_State *L)
{
	createMetatable(L, "PhoenixTibia.Connecion", connectionFunctions);

	luaL_newlib(L, connectionLib);

	return 1;
}

int network_register(lua_State *L)
{
	auto service = checkNetworkService(L);

	lua_pushboolean(L, network->registerService(std::shared_ptr<NetworkService>(service)));

	return 1;
}

int network_unregister(lua_State *L)
{
	auto service = lua_tostring(L, 1);
	lua_pop(L, 1);

	lua_pushboolean(L, network->unregisterService(service));
	return 1;
}

int network_start(lua_State *L)
{
	network->start();

	return 0;
}

int network_stop(lua_State *L)
{
	network->stop();

	return 0;
}

int network_restart(lua_State *L)
{
	network->restart();

	return 0;
}

const luaL_Reg networkLib[] = {
	{ "register", network_register },
	{ "unregister", network_unregister },
	{ "start", network_start },
	{ "stop", network_stop },
	{ "restart", network_restart },
	{ NULL, NULL }
};

int network_registerLib(lua_State *L)
{
	luaL_requiref(L, "packetserializable", packetserializable_registerLib, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "packet", packet_registerLib, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "connection", connection_registerLib, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "networkservice", networkservice_registerLib, 1);
	lua_pop(L, 1);
	luaL_newlib(L, networkLib);
	return 1;
}

void Script::registerFunctions()
{
	luaL_requiref(L, "settings", settings_register, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "manager", manager_register, 1);
	lua_pop(L, 1);
	luaL_requiref(L, "network", network_registerLib, 1);
	lua_pop(L, 1);

	requestRegisterFunctions(L);
}

