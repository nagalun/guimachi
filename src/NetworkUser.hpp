#pragma once

class Network;
class Peer;

class NetworkUser {
	Peer& peer;
	bool online;
	bool owner;
	bool admin;
	bool premium;
	bool suspended;
	
public:
	NetworkUser(Peer&);
	
	Peer& getPeer() const;
	bool isOnline() const;
	bool isOwner() const;
	bool isAdmin() const;
	bool isPremium() const;
	bool isSuspended() const;
	
private:
	void update(bool online, bool owner, bool admin, bool premium, bool suspended);
	
	friend Network;
};
