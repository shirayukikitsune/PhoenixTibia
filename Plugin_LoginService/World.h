#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <string>

#include "Packet.h"

class World
	: public PacketSerializable
{
public:
	//! Used for worlds that will be read from a Packet
	World();
	World(uint32_t id, std::string name, std::string address, unsigned short port);
	virtual ~World();

	virtual void read(Packet &packet);
	virtual void write(Packet &packet);

	const std::string& name();
	boost::asio::ip::tcp::endpoint endpoint();
	uint32_t id();

private:
	std::string m_name;
	boost::asio::ip::tcp::endpoint m_endpoint;
	uint32_t m_id;
};

