#pragma once

#include "NetworkService.h"

#include <lua.hpp>
#include <memory>
#include <vector>

class LuaRunnable
{
public:
	virtual ~LuaRunnable() {
		lua_close(L);
	}

	lua_State* getState() {
		return L;
	}
protected:
	lua_State *L;
};

class LuaNetworkService :
	public NetworkService,
	public std::enable_shared_from_this<LuaNetworkService>,
	public LuaRunnable
{
public:
	void initialize(lua_State *L, const std::string &name);

	virtual std::string getName() const { return name; }

	virtual std::string getBindAddress() { return bindAddress; }
	virtual unsigned short getBindPort() { return bindPort; }

	virtual bool needChecksum() { return checksum; }

	virtual bool canHandle(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handleFirst(NetworkConnectionPtr connection, PacketPtr packet);
	virtual bool handle(NetworkConnectionPtr connection, PacketPtr packet);
	virtual void removeConnection(NetworkConnectionPtr connection);

	std::string bindAddress;
	unsigned short bindPort;
	bool checksum;

private:
	std::string name;
};

class LuaPacketSerializable :
	public PacketSerializable,
	public LuaRunnable
{
	virtual void read(Packet &packet);
	virtual void write(Packet &packet) const;
};
