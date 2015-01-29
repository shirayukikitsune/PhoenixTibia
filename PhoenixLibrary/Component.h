#pragma once

#include <memory>
#include <string>
#include "Callback.h"

class Settings;
class ComponentManager;
class NetworkManager;

class ScriptComponent;
class PluginComponent;
class LoggerComponent;

class Component
	: public std::enable_shared_from_this<Component>
{
public:
	Component() {}

	virtual ~Component() {}

	//! Returns the component name
	virtual const std::string& getName() const; 
	
	//! Initializes the component
	/// @param settings Pointer to Settings, which will affect this component
	bool load(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);
	//! Deinitialize the component
	bool unload();
	//! Reload the component
	virtual bool reload(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);
	//! Returns whether the component is loaded
	bool isLoaded() { return m_loaded; }

	// Conversions to default components, non-default must use dynamic_cast
	virtual ScriptComponent* asScript() { return nullptr; }
	virtual PluginComponent* asPlugin() { return nullptr; }
	virtual LoggerComponent* asLogger() { return nullptr; }

	//! Event fired before loading
	phoenix::callback<true, bool(std::shared_ptr<Component>)> OnLoading;
	//! Event fired when successfully loaded
	phoenix::callback<true, void(std::shared_ptr<Component>)> OnLoaded;
	//! Event fired before unloading
	phoenix::callback<true, bool(std::shared_ptr<Component>)> OnUnloading;
	//! Event fired when successfully unloaded
	phoenix::callback<true, void(std::shared_ptr<Component>)> OnUnloaded;
	//! Event fired before reloading
	phoenix::callback<true, bool(std::shared_ptr<Component>)> OnReloading;
	//! Event fired when successfully reloaded
	phoenix::callback<true, void(std::shared_ptr<Component>)> OnReloaded;

private:
	bool m_loaded;

protected:
	virtual bool internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network) { return true; }
	virtual bool internalUnload() { return true; }
};
