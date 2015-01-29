#pragma once

#include "Component.h"

#include <ostream>
#include <fstream>

enum struct LogLevel {
	None,
	Fatal,
	Error,
	Warning,
	Information,
	Debug,
};

class LoggerComponent :
	public Component
{
public:
	virtual const std::string& getName() const;

	virtual LoggerComponent* asLogger() { return this; }

	void log(LogLevel level, const std::string &message);

private:
	std::ofstream m_outFile;
	LogLevel m_level;

protected:
	virtual bool internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network);
	virtual bool internalUnload();
};
