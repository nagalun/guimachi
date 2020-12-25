#include "NetworkUser.hpp"

#include "Peer.hpp"

#include <iostream>
#include <memory>

NetworkUser::NetworkUser(Peer& peer)
: peer(peer),
  online(false),
  owner(false),
  admin(false),
  premium(false),
  suspended(false) { }

Peer& NetworkUser::getPeer() const {
	return peer;
}

bool NetworkUser::isOnline() const {
	return online;
}

bool NetworkUser::isOwner() const {
	return owner;
}

bool NetworkUser::isAdmin() const {
	return admin;
}

bool NetworkUser::isPremium() const {
	return premium;
}

bool NetworkUser::isSuspended() const {
	return suspended;
}

void NetworkUser::update(bool online, bool owner, bool admin, bool premium, bool suspended) {
	this->online = online;
	this->owner = owner;
	this->admin = admin;
	this->premium = premium;
	this->suspended = suspended;
}
