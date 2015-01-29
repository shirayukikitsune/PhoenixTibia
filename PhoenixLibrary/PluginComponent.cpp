#include "PluginComponent.h"
#include "Settings.h"
#include "Tools.h"
#include "Plugin.h"

#include <sstream>
#include <iostream>
#include <libxml/xmlreader.h>

const std::string& PluginComponent::getName() const
{
	static std::string _name("plugin");
	return _name;
}

bool PluginComponent::hasPlugin(const std::string &name)
{
	return m_plugins.find(name) != m_plugins.end();
}

std::weak_ptr<Plugin> PluginComponent::getPlugin(const std::string &name)
{
#ifdef _DEBUG
	std::string n(name);
	if (n.length() < 6 || n.substr(0, n.length() - 6).compare("-debug") != 0) n.append("-debug");

	auto i = m_plugins.find(n);
#else
	auto i = m_plugins.find(name);
#endif

	if (i == m_plugins.end()) return std::weak_ptr<Plugin>();

	return i->second;
}

bool PluginComponent::internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	std::string path = settings->getString("dataDirectory") + "/plugins";
	std::string file = path + "/" + settings->getString("pluginsFile");
	std::shared_ptr<xmlDoc> doc(xmlParseFile(file.c_str()), XmlDocDeleter());

	if (!doc) {
		std::cout << ">> " << file << " failed to load:" << std::endl << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr root = xmlDocGetRootElement(doc.get());
	if (xmlStrcmp(root->name, (const xmlChar*)"plugins")) {
		std::cout << ">> Invalid XML for plugins" << std::endl;
		return false;
	}

	for (auto node = root->children; node; node = node->next) {
		if (xmlStrcmp(node->name, (const xmlChar*)"plugin"))
			continue;

		std::string s;
		// If missing enabled attribute, defaults to enabled
		if (getXmlAttribute(node, "enabled", s) && !stringToBool(s)) {
			continue;
		}

		if (!getXmlAttribute(node, "id", s)) {
			std::cout << ">> Missing id for plugin" << std::endl;
			continue;
		}

#ifdef _DEBUG
		if (s.length() < 6 || s.substr(0, s.length() - 6).compare("-debug") != 0) s.append("-debug");
#endif

		std::shared_ptr<Plugin> plugin(new Plugin(s));

		std::cout << ">> Initializing plugin " << plugin->getId() << std::endl;

		if (!plugin->load(path, settings, manager, network)) {
			std::cout << ">> Plugin " << plugin->getId() << " failed to initialize" << std::endl;
			continue;
		}

		for (auto child = node->children; child; child = child->next) {
			if (!xmlStrcmp(child->name, (const xmlChar*)"option")) {
				std::string k, v;

				if (!getXmlAttribute(child, "name", k)) {
					std::cout << ">>> Option with no name" << std::endl;
					continue;
				}

				if (!getXmlContent(child, v)) {
					std::cout << ">>> Option with no value" << std::endl;
					continue;
				}

				plugin->setOption(k, v);
			}
		}

		plugin->initialize();
		std::cout << ">> Plugin " << plugin->getId() << " initialized" << std::endl;
		m_plugins[plugin->getId()] = plugin;
	}

	return true;
}


bool PluginComponent::internalUnload()
{
	for (auto &plugin : m_plugins) {
		plugin.second->unload();
	}
	m_plugins.clear();
	return true;
}

