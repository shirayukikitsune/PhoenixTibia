#include "Component.h"
#include <algorithm>

const std::string& Component::getName() const
{
	static std::string _name("unknownComponent");
	return _name;
}


bool Component::load(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	// Fire our loading events
	auto results = OnLoading(shared_from_this());

	// Check if any of the callbacks returned false. If positive, then abort the loading
	if (std::any_of(results.begin(), results.end(), [](const std::pair<int, bool> &r){ return !r.second; }))
		return false;

	// If our internal loading function fails, we do too
	if (!internalLoad(manager, settings, network))
		return false;

	m_loaded = true;

	// Fire our loaded events
	OnLoaded(shared_from_this());
	return true;
}


bool Component::unload()
{
	// Fire our unloading events
	auto results = OnUnloading(shared_from_this());

	// Check if any of the callbacks returned false. If positive, then abort the unloading
	if (std::any_of(results.begin(), results.end(), [](const std::pair<int, bool> &r){ return !r.second; }))
		return false;

	// If our internal unloading function fails, we do too
	if (!internalUnload())
		return false;

	m_loaded = false;

	// Fire our unloaded events
	OnUnloaded(shared_from_this());
	return true;
}


bool Component::reload(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	// Fire our reloading events
	auto results = OnReloading(shared_from_this());

	// Check if any of the callbacks returned false. If positive, then abort the reloading
	if (std::any_of(results.begin(), results.end(), [](const std::pair<int, bool> &r){ return !r.second; }))
		return false;

	// If our internal unloading function fails, we do too
	if (!internalUnload())
		return false;

	m_loaded = false;

	// If our internal loading function fails, we do too
	if (!internalLoad(manager, settings, network))
		return false;

	m_loaded = true;

	// Fire our reloaded events
	OnReloaded(shared_from_this());
	return true;
}
