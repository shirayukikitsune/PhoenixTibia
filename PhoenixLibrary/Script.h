#pragma once

#include <string>
#include <memory>
#include <unordered_map>

class Settings;
class ComponentManager;
class NetworkManager;
struct lua_State;

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

protected:
	virtual void registerFunctions();

private:
	lua_State *L;
	std::unordered_map<std::string, int> m_envs;
};

