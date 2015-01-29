#pragma once

#include "NetworkDefinitions.h"

#include <string>
#include <memory>


//! Interface that defines a service that will be run
class NetworkService
{
public:
	NetworkService();
	virtual ~NetworkService();

	virtual std::string getName() const { return "unknown-service"; }

	virtual std::string getBindAddress();
	virtual unsigned short getBindPort();

	Capability getCapability();

	void setListener(std::shared_ptr<NetworkListener> listener);
	std::shared_ptr<NetworkListener> getListener();

	//! Function that will return whether this service packets needs checksum. All services that does NOT need checksums, should override this function.
	virtual bool needChecksum();

	//! Function that will return whether this service can handle a specific packet.
	virtual bool canHandle(NetworkConnectionPtr connection, PacketPtr packet);

	//! Handles the first packet received on connection
	/**
	 * @returns true if the connection should continue, false if it should be closed
	 */
	virtual bool handleFirst(NetworkConnectionPtr connection, PacketPtr packet);

	//! Handles all packets received on connection, except the first
	/**
	 * @returns true if the connection should continue, false if it should be closed
	 */
	virtual bool handle(NetworkConnectionPtr connection, PacketPtr packet);

	//! Handles when a connection is closed
	virtual void removeConnection(NetworkConnectionPtr connection);

private:
	std::shared_ptr<NetworkListener> m_listener;
};

