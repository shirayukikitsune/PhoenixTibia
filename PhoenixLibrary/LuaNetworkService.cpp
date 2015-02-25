#include "LuaNetworkService.h"

#include "Script.h"

void LuaNetworkService::initialize(lua_State *L, const std::string &name) {
	this->L = lua_newthread(L);
	lua_pop(L, 1);
	this->name = name;
}

bool LuaNetworkService::canHandle(NetworkConnectionPtr connection, PacketPtr packet)
{
	if (lua_getglobal(L, "canHandle") != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return false;
	}

	lua_pushconnection(L, connection.get());
	lua_pushpacket(L, packet.get());

	lua_pcall(L, 2, 1, 0);

	bool result = lua_toboolean(L, 1) != 0;

	return result;
}

bool LuaNetworkService::handleFirst(NetworkConnectionPtr connection, PacketPtr packet)
{
	if (lua_getglobal(L, "handleFirst") != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return false;
	}

	lua_pushconnection(L, connection.get());
	lua_pushpacket(L, packet.get());

	lua_pcall(L, 2, 1, 0);

	bool result = lua_toboolean(L, 1) != 0;

	return result;
}

bool LuaNetworkService::handle(NetworkConnectionPtr connection, PacketPtr packet)
{
	if (lua_getglobal(L, "handle") != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return false;
	}

	lua_pushconnection(L, connection.get());
	lua_pushpacket(L, packet.get());

	lua_pcall(L, 2, 1, 0);

	bool result = lua_toboolean(L, 1) != 0;

	return result;
}

void LuaNetworkService::removeConnection(NetworkConnectionPtr connection)
{
	if (lua_getglobal(L, "removeConnection") != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return;
	}

	lua_pushconnection(L, connection.get());

	lua_pcall(L, 1, 0, 0);
}

void LuaPacketSerializable::read(Packet &packet)
{
	if (lua_getglobal(L, "read") != LUA_TFUNCTION)
	{
		lua_pop(L, 1);
		return;
	}

	lua_pushpacket(L, &packet);

	lua_pcall(L, 1, 0, 0);
}

void LuaPacketSerializable::write(Packet &packet) const
{
	if (lua_getglobal(L, "write") != LUA_TFUNCTION)
	{
		lua_pop(L, 1);
		return;
	}

	lua_pushpacket(L, &packet);

	lua_pcall(L, 1, 0, 0);
}

void LuaPacketSerializable::initialize(lua_State *L)
{
	this->L = L;
}
