#include "Account.h"


Account::Account()
{
}


Account::~Account()
{
}

void Account::read(Packet &packet)
{
	m_username = packet.pop<std::string>();
	m_password = packet.pop<std::string>();
}

void Account::write(Packet &packet)
{
	packet.push<std::string>(m_username).push<std::string>(m_password);
}
