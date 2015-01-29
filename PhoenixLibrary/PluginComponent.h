#pragma once

#include "Component.h"

#include <map>

class Plugin;

class PluginComponent :
	public Component
{
public:
	virtual const std::string& getName() const;

	virtual PluginComponent* asPlugin() { return this; }

	bool hasPlugin(const std::string &name);
	std::weak_ptr<Plugin> getPlugin(const std::string &name);
protected:
	virtual bool internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);
	virtual bool internalUnload();

	std::map<std::string, std::shared_ptr<Plugin>> m_plugins;
};

