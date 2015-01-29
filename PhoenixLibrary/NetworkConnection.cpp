#include "NetworkConnection.h"
#include "NetworkService.h"

#include <iostream>

using boost::asio::ip::tcp;

NetworkConnection::NetworkConnection(boost::asio::io_service &ioService)
: m_socket(ioService), m_timeout(ioService)
{
	m_hasKeys = false;
}


NetworkConnection::~NetworkConnection()
{
	close();
}

void NetworkConnection::close()
{
	boost::system::error_code ec;
	if (m_socket.is_open()) {
		m_socket.shutdown(tcp::socket::shutdown_both, ec);
		if (ec) std::cout << "NetworkConnection::shutdown: error: " << ec.message() << std::endl;
	}
	m_socket.close(ec);
	m_timeout.cancel();
	if (ec) std::cout << "NetworkConnection::close: error: " << ec.message() << std::endl;
}

void NetworkConnection::setKeys(std::array<uint32_t, 4> &keys)
{
	m_hasKeys = true;

	std::copy(keys.begin(), keys.end(), m_keys.begin());
}

void NetworkConnection::setTimeout(std::int64_t msec)
{
	m_timeout.expires_from_now(boost::posix_time::millisec(msec));

	m_timeout.async_wait([this](boost::system::error_code error) {
		if (!error) {
			close();
		}
	});
}

void NetworkConnection::unsetTimeout()
{
	m_timeout.cancel();
}

uint32_t NetworkConnection::getLastChecksum()
{
	return m_checksum;
}

void NetworkConnection::beginReading(NetworkConnection::ReadHandler handler)
{
	m_handler = handler;
	setTimeout(NetworkConnection::readTimeout);
	boost::asio::async_read(m_socket, boost::asio::buffer(m_recvPacket.data(), Packet::headerSize), std::bind(&NetworkConnection::handleHeader, shared_from_this(), std::placeholders::_1));
}

void NetworkConnection::send(PacketPtr packet, std::function<void(boost::system::error_code, size_t)> handler)
{
	if (!handler) handler = [](boost::system::error_code, size_t) {};

	if (m_hasKeys) {
		packet->writeMessageLength();
		encrypt(packet);
	}
	packet->addCryptoHeader(needChecksum());

	boost::asio::async_write(m_socket, boost::asio::buffer(packet->data() + packet->start(), packet->size()), handler);
}

void NetworkConnection::handleHeader(boost::system::error_code error)
{
	if (!error) {
		setTimeout(NetworkConnection::readTimeout);
		auto len = m_recvPacket.parseLength();
		m_recvPacket.pos(Packet::headerSize); // Rewind the packet

		boost::asio::async_read(m_socket, boost::asio::buffer(m_recvPacket.data() + Packet::headerSize, len), std::bind(&NetworkConnection::handleBody, shared_from_this(), std::placeholders::_1));
	}
	else unsetTimeout();
}

void NetworkConnection::handleBody(boost::system::error_code error)
{
	if (!error) {
		if (m_hasKeys && !decrypt()) {
			return;
		}

		uint32_t checksumReceived = m_recvPacket.peek<uint32_t>(), length = m_recvPacket.size() - m_recvPacket.pos() - 4;
		if (length > 0) m_checksum = adlerChecksum(m_recvPacket.data() + m_recvPacket.pos() + 4, length);
		else m_checksum = 0;

		setTimeout(NetworkConnection::readTimeout);
		m_handler(std::make_shared<Packet>(m_recvPacket), error);
	}
	else unsetTimeout();
}

void NetworkConnection::encrypt(std::shared_ptr<Packet> packet)
{
	const uint32_t delta = 0x61C88647;

	// The message must be a multiple of 8 
	size_t paddingBytes = packet->size() % 8;
	if (paddingBytes != 0) {
		packet->pad(8 - paddingBytes);
	}

	uint32_t* buffer = reinterpret_cast<uint32_t*>(packet->data() + packet->start());
	const size_t messageLength = packet->size() / 4;
	size_t readPos = 0;
	const uint32_t k[] = { m_keys[0], m_keys[1], m_keys[2], m_keys[3] };
	while (readPos < messageLength) {
		uint32_t v0 = buffer[readPos], v1 = buffer[readPos + 1];
		uint32_t sum = 0;

		for (int32_t i = 32; --i >= 0;) {
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum -= delta;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[(sum >> 11) & 3]);
		}

		buffer[readPos++] = v0;
		buffer[readPos++] = v1;
	}
}

bool NetworkConnection::decrypt()
{
	if (((m_recvPacket.size() - 6) & 7) != 0) {
		return false;
	}

	const uint32_t delta = 0x61C88647;

	uint32_t* buffer = reinterpret_cast<uint32_t*>(m_recvPacket.data() + m_recvPacket.pos());
	const size_t messageLength = (m_recvPacket.size() - 6) / 4;
	size_t readPos = 0;
	const uint32_t k[] = { m_keys[0], m_keys[1], m_keys[2], m_keys[3] };
	while (readPos < messageLength) {
		uint32_t v0 = buffer[readPos], v1 = buffer[readPos + 1];
		uint32_t sum = 0xC6EF3720;

		for (int32_t i = 32; --i >= 0;) {
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[(sum >> 11) & 3]);
			sum += delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}

		buffer[readPos++] = v0;
		buffer[readPos++] = v1;
	}

	size_t innerLength = m_recvPacket.pop<uint16_t>();
	if (innerLength > m_recvPacket.size() - 8) {
		return false;
	}

	m_recvPacket.size(innerLength);
	return true;
}

bool NetworkConnection::needChecksum()
{
	return m_service && m_service->needChecksum();
}
