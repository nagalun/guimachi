#pragma once

#include <ostream>
#include <string>
#include <functional>
#include <memory>
#include <chrono>
#include <QHostAddress>
#include <QTimer>

#include "Promise.hpp"
#include "types.hpp"

class EngineInterface;

class Peer {
public:
	class Endpoint;
	enum class TunState {
		OFFLINE,
		BLOCKED,
		SERVER,
		RELAYED,
		DIRECT
	};

private:
	EngineInterface &ei;
	const str id;
	str nick;

	QHostAddress tunIpv4;
	QHostAddress tunIpv6;
	QHostAddress aliasIpv4;

	/*QHostAddress pubIpv4;
	u16 pubPort;*/

	u32 peerBits;
	u8 authStatus;

	u32 tunBits;
	bool tunAvailable;
	bool tunHasActivity;
	bool pendingMessage;
	std::vector<Endpoint> endpoints;
	
	std::chrono::system_clock::time_point lastTypingSentOn;
	QTimer activityTimeout;
	std::function<void()> extraActivityCb;

public:
	Peer(EngineInterface &, str id);

	bool isChatEnabled() const;
	bool isIpv6Enabled() const;
	bool isIpv4Enabled() const;

	bool hasPendingMessage() const;
	bool hasAliasSet() const;
	bool isTunnelReady() const;
	bool isTunnelActive() const;
	bool isTunnelBad() const;
	u32 getTunnelBits() const;
	TunState getTunnelState() const;

	u16 getAuthState() const; // should be u8, but be u16 to avoid displaying as a char
	u32 getPeerBits() const;
	const std::string& getId() const;
	const std::string& getNick() const;
	const QHostAddress& getRealAddress4() const;
	const QHostAddress& getAddress4() const; // will give alias if set
	const QHostAddress& getAddress6() const;
	const Endpoint& getCurrentEndpoint() const;
	
	void clearPendingMessage();

	void sendTyping();
	void resetTypingTimeout();
	std::shared_ptr<Promise<bool, u8>> sendMessage(std::string);

	bool operator==(const Peer &) const;
	bool operator <(const Peer &) const;

private:
	void update(str nick, QHostAddress tun4, QHostAddress tun6, QHostAddress alias4, u32 pbits, u8 auth, u32 tbits, bool tavail);
	void setEndpoints(std::vector<Endpoint>);
	void setExtraActivityCb(std::function<void()>);
	void setTunIsActive();
	void setPendingMessage();
	
	friend EngineInterface;
};

class Peer::Endpoint {
	const u32 id;
	u8 type;
	u8 status;

	u16 port;
	QHostAddress remoteIpv4;

	bool tcp;
	bool active;

public:
	Endpoint(u32 id, u8 type, u8 status, bool tcp, QHostAddress, u16 port, bool active);
	Endpoint();

	u8 getType() const;
	bool isCurrentlyUsed() const;
	bool isActive() const;
	bool isTcp() const;

	const QHostAddress &address() const;
	QString addressToString() const;
};

bool operator  <(std::reference_wrapper<Peer> a, std::reference_wrapper<Peer> b);
bool operator ==(std::reference_wrapper<Peer> a, std::reference_wrapper<Peer> b);

const char * toString(const Peer::TunState state);
QString toQString(const Peer::TunState state);
std::ostream& operator<<(std::ostream &o, const Peer::TunState state);

namespace std {
	template<>
	struct hash<Peer> {
		std::size_t operator()(const Peer &p) const {
			return std::hash<std::string>{}(p.getId());
		}
	};

	template<>
	struct hash<std::reference_wrapper<Peer>> {
		std::size_t operator()(const std::reference_wrapper<Peer> &p) const {
			return std::hash<Peer>{}(p.get());
		}
	};
}
