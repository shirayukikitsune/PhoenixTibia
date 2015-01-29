#pragma once

#include <string>

struct lua_State;

class Settings
{
public:
	Settings(const std::string &file);
	~Settings();

	bool load();

	std::string getString(const std::string &key);
	double getNumber(const std::string &key);
	long long getInteger(const std::string &key);
	unsigned long long getUnsigned(const std::string &key);

private:
	bool m_ready;
	lua_State *m_luaState;
	std::string m_file;
};

