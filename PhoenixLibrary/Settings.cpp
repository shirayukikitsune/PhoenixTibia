#include "Settings.h"

#include <lua.hpp>


Settings::Settings(const std::string &file)
: m_file(file)
{
	m_ready = false;
	m_luaState = nullptr;
}


Settings::~Settings()
{
	if (m_ready) lua_close(m_luaState);
}


bool Settings::load()
{
	if (m_ready) lua_close(m_luaState);

	m_luaState = luaL_newstate();
	if (!m_luaState) return false;

	luaL_openlibs(m_luaState);
	if (luaL_dofile(m_luaState, m_file.c_str()))
	{
		lua_close(m_luaState);
		m_luaState = nullptr;
		return false;
	}

	m_ready = true;

	return true;
}

std::string Settings::getString(const std::string &key)
{
	if (!m_ready) return "";

	lua_getglobal(m_luaState, key.c_str());
	std::string ret = "";
	if (lua_isstring(m_luaState, -1)) ret = lua_tostring(m_luaState, -1);
	lua_pop(m_luaState, 1);
	return ret;
}

lua_Number Settings::getNumber(const std::string &key)
{
	if (!m_ready) return 0.0;

	lua_getglobal(m_luaState, key.c_str());
	lua_Number ret = 0.0;
	if (lua_isnumber(m_luaState, -1)) ret = lua_tonumber(m_luaState, -1);
	lua_pop(m_luaState, 1);
	return ret;
}

lua_Integer Settings::getInteger(const std::string &key)
{
	if (!m_ready) return 0;

	lua_getglobal(m_luaState, key.c_str());
	lua_Integer ret = 0;
	if (lua_isnumber(m_luaState, -1)) ret = lua_tointeger(m_luaState, -1);
	lua_pop(m_luaState, 1);
	return ret;
}

lua_Unsigned Settings::getUnsigned(const std::string &key)
{
	if (!m_ready) return 0;

	lua_getglobal(m_luaState, key.c_str());
	lua_Unsigned ret = 0;
	if (lua_isnumber(m_luaState, -1)) ret = (lua_Unsigned)lua_tonumber(m_luaState, -1);
	lua_pop(m_luaState, 1);
	return ret;
}
