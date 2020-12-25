#pragma once

#include <utility>
#include <memory>
#include <unordered_map>

#include "Promise.hpp"
#include "Peer.hpp"
#include "types.hpp"
#include "NetworkUser.hpp"

class EngineInterface;

class Network {
	EngineInterface& ei;
	const str id;
	str name;
	str owner;

	u32 maxMembers;
	
	str loginMsg;
	str domain;
	str linkedAccount;

	std::unordered_map<str, NetworkUser> peers;

	bool leavingAllowed;
	bool connected;
	bool pendingMessage;
	
public:
	Network(EngineInterface&, str id);
	
	const std::string& getId() const;
	const std::string& getName() const;
	const std::string& getOwner() const; // returns either the linked account (if exists) or owner id
	const std::string& getOwnerId() const;
	u32 getSize() const; // amount of users
	u32 getCapacity() const; // max users
	const std::string& getLoginMessage() const;
	const std::string& getDomain() const;
	const std::string& getLinkedAccount() const;
	bool canLeave() const;
	bool isConnected() const;
	bool hasPendingMessage() const;
	void forEach(std::function<void(const NetworkUser&)>) const;
	const std::unordered_map<str, NetworkUser>& getPeerMap() const;
	
	void clearPendingMessage();

	void disconnect() const; // technically not const but whatever
	void connect() const;
	void leave() const;
	std::shared_ptr<Promise<bool, u8>> sendMessage(std::string) const;
	
private:
	std::pair<bool, const NetworkUser&> updatePeer(Peer&, bool online, bool owner, bool admin, bool premium, bool suspended);
	NetworkUser& getPeer(Peer&);
	void removePeer(const Peer&);
	void update(str name, str owner, u32 maxMembers, str loginMsg, str domain, str linkedAccount, bool leavingAllowed, bool connected);
	
	void setPendingMessage();

	friend EngineInterface;
};
