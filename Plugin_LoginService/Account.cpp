#include "Account.h"


Account::Account()
{
	m_success = false;
}


Account::~Account()
{
}

void Account::read(Packet &packet)
{
	m_success = packet.pop<uint8_t>() != 0;
	m_username = packet.pop<std::string>();
	m_password = packet.pop<std::string>();
}

void Account::write(Packet &packet) const
{
	packet.push<uint8_t>(m_success ? 1 : 0).push<std::string>(m_username).push<std::string>(m_password);
}

const std::string& Account::username() const
{
	return m_username;
}

void Account::username(const std::string &name)
{
	m_username = name;
}

const std::string& Account::password() const
{
	return m_password;
}

void Account::password(const std::string &name)
{
	m_password = name;
}

bool Account::success() const
{
	return m_success;
}
