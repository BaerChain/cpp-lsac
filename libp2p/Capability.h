#pragma once

#include "Common.h"

namespace dev
{
namespace p2p
{
/**
 * @brief The Capability interface.
 * This interface has to be implemented to support any devp2p subprotocol.
 */

class CapabilityFace
{
public:
    virtual ~CapabilityFace() = default;

    /// Subprotocol name, used in negotiating common capabilities with the peers.
    virtual std::string name() const = 0;
    /// Subprotocol version, used in negotiating common capabilities with the peers.
    virtual u256 version() const = 0;
    /// Number of messages supported by the subprotocol version.
    virtual unsigned messageCount() const = 0;
    
    // get message offet
	virtual unsigned offset() const = 0;

    /// Called by the Host when capability is activated
    /// (usually when network communication is being enabled)
    virtual void onStarting() = 0;
    /// Called by the Host when capability is deactivated
    /// (usually when network communication is being disabled)
    virtual void onStopping() = 0;

    /// Called by the Host when new peer is connected.
    /// Guaranteed to be called first before any interpretCapabilityPacket for this peer.
    virtual void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) = 0;
    /// Called by the Host when the messaege is received from the peer
    /// @returns true if the message was interpreted, false if the message had not supported type.
    virtual bool interpretCapabilityPacket(NodeID const& _nodeID, unsigned _id, RLP const&) = 0;
    /// Called by the Host when the peer is disconnected.
    /// Guaranteed to be called last after any interpretCapabilityPacket for this peer.
    virtual void onDisconnect(NodeID const& _nodeID) = 0;
    /// Background work loop - called by the host every backgroundWorkInterval()
    virtual void doBackgroundWork() = 0;
    virtual std::chrono::milliseconds backgroundWorkInterval() const = 0;

};

}  // namespace p2p

}  // namespace dev
