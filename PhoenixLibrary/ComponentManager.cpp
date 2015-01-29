#include "ComponentManager.h"
#include "Component.h"
#include <iostream>

// Default components here
#include "ScriptComponent.h"
#include "PluginComponent.h"
#include "LoggerComponent.h"


ComponentManager::ComponentManager()
{
}


ComponentManager::~ComponentManager()
{
}


void ComponentManager::registerDefaultComponents()
{
	std::cout << "> Registering default components" << std::endl;

	std::shared_ptr<Component> component(new ScriptComponent);
	this->registerComponent(component);
	component.reset(new PluginComponent);
	this->registerComponent(component);
	component.reset(new LoggerComponent);
	this->registerComponent(component);
}


void ComponentManager::registerComponent(std::shared_ptr<Component> component)
{
	// Can't register an invalid pointer
	if (component == nullptr)
		return;

	std::cout << ">> Registering component: " << component->getName() << std::endl;

	// Check if we have a component with the same name registered
	auto i = m_components.find(component->getName());

	if (i == m_components.end()) {
		// Not registered, then register!
		m_components.emplace(component->getName(), component);
	}
	else {
		// Already registered; then unload the previous component and register this
		if (i->second->isLoaded()) {
			i->second->unload();
		}

		i->second = component;
	}
}


bool ComponentManager::loadComponents(std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	std::cout << "> Loading registered components" << std::endl;

	for (auto component : m_components)
	{
		if (!component.second->load(shared_from_this(), settings, network)) {
			std::cout << ">> Component \"" << component.first << "\" failed to load" << std::endl;
			return false;
		}
		else {
			std::cout << ">> Loaded \"" << component.first << "\"" << std::endl;
		}
	}

	return true;
}


void ComponentManager::unloadComponents()
{
	std::cout << "> Unloading registered components" << std::endl;

	for (auto component : m_components)
	{
		std::cout << ">> Unloading \"" << component.first << "\"" << std::endl;
		if (!component.second->unload()) {
			std::cout << ">> Component \"" << component.first << "\" failed to unload" << std::endl;
		}
		else {
			std::cout << ">> Unloaded \"" << component.first << "\"" << std::endl;
		}
	}

	m_components.clear();
}

