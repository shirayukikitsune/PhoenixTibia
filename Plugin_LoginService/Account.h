#pragma once

#include "Packet.h"

#include <string>
#include <list>

class Character;

class Account
	: public PacketSerializable
{
public:
	Account();
	~Account();

	virtual void read(Packet &packet);
	virtual void write(Packet &packet);

	const std::string& username();
	const std::string& password();

private:
	std::string m_username;
	std::string m_password;
};

