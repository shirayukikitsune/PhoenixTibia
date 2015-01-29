#include "Tools.h"
#include "Packet.h"

#include <libxml/xmlreader.h>
#include <cstring>
#include <sstream>

void XmlDocDeleter::operator()(void *p)
{
	xmlFreeDoc((xmlDocPtr)p);
}


bool stringToBool(const std::string &in)
{
	if (in.empty()) return false;

	char f = tolower(in[0]);
	if (f == 'y' || f == 't') return true;
	if (f >= '0' && f <= '9') {
		if (std::stoi(in) != 0) return true;
	}

	return false;
}


std::string getXmlAttribute(_xmlNode* node, const std::string& attribute, const std::string& _default)
{
	std::string value;
	if (!getXmlAttribute(node, attribute, value)) return _default;

	return value;
}


bool getXmlAttribute(_xmlNode* node, const std::string& attribute, std::string& output)
{
	if (!node || attribute.empty()) return false;

	char* value = (char*)xmlGetProp(node, (const xmlChar*)attribute.c_str());
	if (!value) return false;

	output = value;

	xmlFree(value);
	return true;
}

bool getXmlContent(_xmlNode* node, std::string& output)
{
	if (!node)
		return false;

	char* content = (char*)xmlNodeGetContent(node);
	if (!content) return false;

	output = content;

	xmlFree(content);
	return true;
}

uint32_t adlerChecksum(uint8_t* data, size_t length)
{
	if (length > Packet::maxPacketSize || !length)
		return 0;

	const uint16_t adler = 65521;
	uint32_t a = 1, b = 0;
	while (length > 0)
	{
		size_t tmp = length > 5552 ? 5552 : length;
		length -= tmp;
		do
		{
			a += *data++;
			b += a;
		} while (--tmp);
		a %= adler;
		b %= adler;
	}

	return (b << 16) | a;
}

std::string getLastXMLError()
{
	std::stringstream ss;
	xmlErrorPtr lastError = xmlGetLastError();
	if (lastError->line)
		ss << "Line: " << lastError->line << ", ";

	ss << "Info: " << lastError->message << std::endl;
	return ss.str();
}
