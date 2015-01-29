#pragma once

#include <memory>
#include <string>

#include "Packet.h"

class World;

class Character
	: public PacketSerializable
{
public:
	Character();
	~Character();

	virtual void read(Packet &packet);
	virtual void write(Packet &packet);

	const std::string& name();
	std::shared_ptr<World> world();

private:
	std::string m_name;
	std::shared_ptr<World> m_world;
};

