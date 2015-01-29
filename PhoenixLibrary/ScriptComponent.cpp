#include "ScriptComponent.h"
#include "ComponentManager.h"
#include "Settings.h"
#include "Tools.h"
#include "Script.h"

#include <iostream>
#include <thread>
#include <list>
#include <boost/filesystem.hpp>
#include <libxml/xmlreader.h>

ScriptComponent::ScriptComponent()
{

}

const std::string& ScriptComponent::getName() const
{
	static std::string _name("script");
	return _name;
}

bool ScriptComponent::internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	// Load all scripts
	std::string path = settings->getString("dataDirectory") + "/scripts";
	std::string file = path + "/" + settings->getString("scriptsFile");
	std::shared_ptr<xmlDoc> doc(xmlParseFile(file.c_str()), XmlDocDeleter());

	if (!doc) {
		std::cout << ">> " << file << " failed to load:" << std::endl << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr root = xmlDocGetRootElement(doc.get());
	if (xmlStrcmp(root->name, (const xmlChar*)"scripts")) {
		std::cout << ">> Invalid XML for scripts" << std::endl;
		return false;
	}

	m_scripts.reset(new Script());
	m_scripts->initialize(path, settings, manager, network);

	for (auto node = root->children; node; node = node->next) {
		if (xmlStrcmp(node->name, (const xmlChar*)"script"))
			continue;

		std::string id;
		// If missing enabled attribute, defaults to enabled
		if (getXmlAttribute(node, "enabled", id) && !stringToBool(id)) {
			continue;
		}

		if (!getXmlAttribute(node, "id", id)) {
			std::cout << ">> Missing id for script" << std::endl;
			continue;
		}

		std::cout << ">> Initializing script " << id << std::endl;

		if (!m_scripts->load(path + "/" + id + ".lua", id)){
			std::cout << ">> Script " << id << " failed to initialize" << std::endl;
			continue;
		}

		if (m_scripts->prepareCall(id, "onLoaded") && m_scripts->call(0, 0)) {
			std::cout << ">> Script " << id << " initialized" << std::endl;
		}
		else std::cout << ">> Script " << id << " error on calling onLoaded (script still active)" << std::endl;
	}

	return true;
}

bool ScriptComponent::internalUnload()
{
	m_scripts.reset();
	return true;
}

