#include "LoggerComponent.h"
#include "Settings.h"

#include <ctime>
#include <mutex>
#include <array>
#include <locale>
#include <codecvt>

const std::string& LoggerComponent::getName() const
{
	static std::string _name("logger");
	return _name;
}

inline std::ostream& __CLRCALL_OR_CDECL timestamp(std::ostream& _Ostr)
{
	auto now = std::time(nullptr);
	auto local = std::localtime(&now);
	std::array<char, 512> buffer;
	std::strftime(buffer.data(), buffer.size(), "[%#c]", local);
	_Ostr << buffer.data();
	return _Ostr;
};

void LoggerComponent::log(LogLevel level, const std::string &message)
{
	if (m_outFile.good() && level <= m_level) {
		m_outFile << timestamp << " " << message << std::endl;
		m_outFile.flush();
	}
}

bool LoggerComponent::internalLoad(std::shared_ptr<ComponentManager> manager, std::shared_ptr<Settings> settings, std::shared_ptr<NetworkManager> network)
{
	m_level = (LogLevel)settings->getUnsigned("loggerLevel");

	if (m_level == LogLevel::None)
		return true;

	m_outFile.open(settings->getString("loggerFile"));

	return m_outFile.good();
}

bool LoggerComponent::internalUnload()
{
	if (m_outFile.is_open())
		m_outFile.close();

	return true;
}

