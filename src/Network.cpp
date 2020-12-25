#include "Network.hpp"

#include "EngineInterface.hpp"

Network::Network(EngineInterface& ei, str id)
: ei(ei),
  id(std::move(id)),
  maxMembers(0),
  leavingAllowed(false),
  connected(false),
  pendingMessage(false) { }

const std::string& Network::getId() const {
	return id;
}

const std::string& Network::getName() const {
	return name;
}

const std::string& Network::getOwner() const {
	return linkedAccount.size() ? linkedAccount : owner;
}

const std::string& Network::getOwnerId() const {
	return owner;
}

u32 Network::getSize() const {
	return peers.size() + 1; // We count as a member too!
}

u32 Network::getCapacity() const {
	return maxMembers;
}

const std::string& Network::getLoginMessage() const {
	return loginMsg;
}

const std::string& Network::getDomain() const {
	return domain;
}

const std::string& Network::getLinkedAccount() const {
	return linkedAccount;
}

bool Network::canLeave() const {
	return leavingAllowed;
}

bool Network::isConnected() const {
	return connected;
}

bool Network::hasPendingMessage() const {
	return pendingMessage;
}

void Network::forEach(std::function<void(const NetworkUser&)> f) const {
	for (const auto& i : peers) {
		f(i.second);
	}
}

const std::unordered_map<str, NetworkUser>& Network::getPeerMap() const {
	return peers;
}

void Network::clearPendingMessage() {
	pendingMessage = false;
}

void Network::disconnect() const {
	ei.netwDisconnect(id);
}

void Network::connect() const {
	ei.netwConnect(id);
}

void Network::leave() const {
	ei.netwLeave(id);
}

std::shared_ptr<Promise<bool, u8>> Network::sendMessage(std::string str) const {
	if (isConnected()) {
		return ei.imSendMessage(true, id, std::move(str));
	} else {
		auto p = std::make_shared<Promise<bool, u8>>();
		p->reject(false); // already trying to send a message, wait until it's sent! TODO: enum
		return p;
	}
}

std::pair<bool, const NetworkUser&> Network::updatePeer(Peer& p, bool online, bool owner, bool admin, bool premium, bool suspended) {
	auto res = peers.try_emplace(p.getId(), p);
	NetworkUser& nu = res.first->second;
	nu.update(online, owner, admin, premium, suspended);
	
	return {res.second, nu};
}

NetworkUser& Network::getPeer(Peer& p) {
	return peers.at(p.getId());
}


void Network::removePeer(const Peer& p) {
	peers.erase(p.getId());
}

void Network::update(str name, str owner, u32 maxMembers, str loginMsg, str domain, str linkedAccount, bool leavingAllowed, bool connected) {
	this->name = std::move(name);
	this->owner = std::move(owner);
	this->maxMembers = maxMembers;
	this->loginMsg = std::move(loginMsg);
	this->domain = std::move(domain);
	this->linkedAccount = std::move(linkedAccount);
	this->leavingAllowed = leavingAllowed;
	this->connected = connected;
}

void Network::setPendingMessage() {
	pendingMessage = true;
}
