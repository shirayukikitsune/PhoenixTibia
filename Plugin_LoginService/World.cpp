#include "World.h"

using namespace boost::asio::ip;

World::World()
{
	m_id = 0;
}

World::World(uint32_t id, std::string name, std::string address, unsigned short port)
: m_id(id), m_name(name), m_endpoint(address::from_string(address), port)
{
}

World::~World()
{
}

void World::read(Packet &packet)
{
	m_name = packet.pop<std::string>();
	m_endpoint = tcp::endpoint(address::from_string(packet.pop<std::string>()), packet.pop<unsigned short>());
	m_id = packet.pop<uint32_t>();
}

void World::write(Packet &packet)
{
	packet.push<std::string>(name()).push<std::string>(m_endpoint.address().to_string()).push<unsigned short>(m_endpoint.port()).push<uint32_t>(m_id);
}

const std::string& World::name()
{
	return m_name;
}

tcp::endpoint World::endpoint()
{
	return m_endpoint;
}

uint32_t World::id()
{
	return m_id;
}
