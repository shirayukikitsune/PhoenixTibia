#pragma once

#include <string>
#include <cstdint>

struct XmlDocDeleter
{
	void operator() (void* p);
};

bool stringToBool(const std::string &in);

struct _xmlNode;
std::string getXmlAttribute(_xmlNode* node, const std::string& attribute, const std::string& _default = "");
bool getXmlAttribute(_xmlNode* node, const std::string& attribute, std::string& output);
bool getXmlContent(_xmlNode* node, std::string& output);
uint32_t adlerChecksum(uint8_t* data, size_t length);
std::string getLastXMLError();
