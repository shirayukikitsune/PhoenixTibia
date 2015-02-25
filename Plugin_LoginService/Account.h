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
	virtual void write(Packet &packet) const;

	const std::string& username() const;
	void username(const std::string &name);
		
	const std::string& password() const;
	void password(const std::string &name);

	bool success() const;

private:
	std::string m_username;
	std::string m_password;
	bool m_success;
};

