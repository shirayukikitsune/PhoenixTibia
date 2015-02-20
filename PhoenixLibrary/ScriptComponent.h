#pragma once

#include "Component.h"

#include <vector>
#include <map>
#include <lua.hpp>

class Settings;
class Script;

class ScriptComponent :
	public Component
{
public:
	ScriptComponent();

	virtual const std::string& getName() const;

	virtual ScriptComponent* asScript() { return this; }

	std::shared_ptr<Script> getScript() { return m_scripts; }

protected:
	virtual bool internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);
	virtual bool internalUnload();

private:
	std::shared_ptr<Script> m_scripts;
};

