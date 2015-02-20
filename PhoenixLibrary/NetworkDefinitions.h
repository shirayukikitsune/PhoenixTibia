#pragma once

#if (defined WINDOWS || defined _WIN32 || defined WIN32 || defined _WIN64 || defined WIN64 || defined __WINDOWS__) && !defined _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

// We won't need boost::exception
#define BOOST_EXCEPTION_DISABLE

#include <boost/asio.hpp>
#include <memory>

#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif

#include "Packet.h"

class NetworkService;
class NetworkManager;
class NetworkListener;
class NetworkConnection;

//! Class that defines a capability of a service
class Capability
	: public PacketSerializable
{
public:
	Capability() {}
	Capability(const std::string &_name)
		: name(_name) {
	}
	Capability(const std::string &_name, boost::asio::ip::tcp::endpoint _ep, std::weak_ptr<NetworkConnection> _c)
		: name(_name), connection(_c) {
		serviceEndpoint = _ep;
	}

	//! The name of the capability
	std::string name;
	//! The service endpoint
	boost::asio::ip::tcp::endpoint serviceEndpoint;
	//! The connection to the interserver service. 
	std::weak_ptr<NetworkConnection> connection;

	bool operator<(const Capability &other) const {
		return name < other.name;
	}
	bool operator==(const Capability &other) const {
		return name == other.name;
	}

	virtual void read(Packet &packet) {
		name = packet.pop<std::string>();
		serviceEndpoint.address(boost::asio::ip::address::from_string(packet.pop<std::string>()));
		serviceEndpoint.port(packet.pop<uint16_t>());
	}
	virtual void write(Packet &packet) const { 
		packet.push<std::string>(name).push<std::string>(serviceEndpoint.address().to_string()).push<uint16_t>(serviceEndpoint.port());
	}
};

typedef std::shared_ptr<NetworkConnection> NetworkConnectionPtr;
typedef std::shared_ptr<Packet> PacketPtr;