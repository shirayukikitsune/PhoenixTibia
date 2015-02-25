#pragma once

#include "Callback.h"

#include <lua.hpp>
#include <memory>
#include <unordered_map>
#include <string>

class ComponentManager;
class Packet;
class PacketSerializable;
class Settings;
class NetworkConnection;
class NetworkManager;

class Script
{
public:
	Script();
	virtual ~Script();

	bool initialize(const std::string &path, std::shared_ptr<Settings> settings, std::shared_ptr<ComponentManager> manager, std::shared_ptr<NetworkManager> network);
	void deinitialize();

	bool load(const std::string &path, const std::string &scriptId);
	bool unload(const std::string &scriptId);

	bool prepareCall(const std::string &scriptId, const std::string &function);
	bool call(int nArgs, int nResults = -1); // -1 = LUA_MULTRET

	lua_State* getEnv() { return L; }

	phoenix::callback<true, void(lua_State *L)> requestRegisterFunctions;

protected:
	virtual void registerFunctions();

private:
	lua_State *L;
	std::unordered_map<std::string, int> m_envs;
};

void copyFunction(lua_State *fromState, lua_State *toState);
int createMetatable(lua_State *L, const char *tableName, const luaL_Reg *functions);
Packet* lua_topacket(lua_State *L, int index);
PacketSerializable* lua_topacketserializable(lua_State *L, int index);
NetworkConnection* checkNetworkConnection(lua_State *L);
int lua_pushpacket(lua_State *L, Packet* packet);
int lua_pushconnection(lua_State *L, NetworkConnection* connection);
int lua_pushpacketserializable(lua_State *L, PacketSerializable* data);
