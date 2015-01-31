#include "Character.h"
#include "World.h"

extern std::map<uint32_t, std::shared_ptr<World>> g_worlds;

Character::Character()
{
}

Character::~Character()
{
}

void Character::read(Packet &packet)
{
	m_name = packet.pop<std::string>();
	auto iWorld = g_worlds.find(packet.pop<uint32_t>());
	if (iWorld == g_worlds.end()) m_world = nullptr;
	else m_world = iWorld->second;
}

void Character::write(Packet &packet)
{
	packet.push<std::string>(m_name).push<uint32_t>(m_world ? m_world->id() : 0);
}

const std::string& Character::name()
{
	return m_name;
}

std::shared_ptr<World> Character::world()
{
	return m_world;
}
