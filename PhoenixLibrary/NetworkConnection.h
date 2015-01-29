#pragma once

#include "NetworkDefinitions.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include "Packet.h"

class NetworkConnection
	: public std::enable_shared_from_this<NetworkConnection>
{
public:
	enum { readTimeout = 10000 };
	typedef std::function<void(std::shared_ptr<Packet>, boost::system::error_code)> ReadHandler;

	NetworkConnection(boost::asio::io_service &ioService);
	~NetworkConnection();

	virtual void close();

	boost::asio::ip::tcp::socket& socket() { return m_socket; }

	void service(std::shared_ptr<NetworkService> service) { m_service = service; }
	std::shared_ptr<NetworkService> service() { return m_service; }

	void beginReading(ReadHandler handler);
	void send(std::shared_ptr<Packet> packet, std::function<void(boost::system::error_code, size_t)> handler = nullptr);

	uint32_t getLastChecksum();

	void setKeys(std::array<uint32_t, 4> &keys);

private:
	boost::asio::deadline_timer m_timeout;
	boost::asio::ip::tcp::socket m_socket;
	std::shared_ptr<NetworkService> m_service;
	ReadHandler m_handler;
	Packet m_recvPacket;
	uint32_t m_checksum;
	std::array<uint32_t, 4> m_keys;
	bool m_hasKeys;
	
	void handleHeader(boost::system::error_code error);
	void handleBody(boost::system::error_code error);

	void encrypt(std::shared_ptr<Packet> packet);
	bool decrypt();

	void unsetTimeout();

protected:
	void setTimeout(std::int64_t msec);
	virtual bool needChecksum();
};

