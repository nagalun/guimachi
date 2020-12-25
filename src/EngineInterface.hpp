#pragma once

#include <unordered_map>
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

#include <QObject>
#include <QHostAddress>

#include "misc/explints.hpp"
#include "EngineSocket.hpp"
#include "EngineState.hpp"
#include "Network.hpp"
#include "Peer.hpp"
#include "opcodes.hpp"
#include "Promise.hpp"

class Packet;

class EngineInterface : public QObject {
	Q_OBJECT
	
	EngineSocket s;
	std::unordered_map<u8, std::function<void(u8 *, sz_t)>> handlers;
	std::unordered_map<str, Peer> peers;
	std::unordered_map<str, Network> networks;

	std::map<str, std::shared_ptr<Promise<bool, u8>>> imPromises;

	bool batchUpdate;
	
	u32 selfBits;
	QHostAddress selfTunIpv4;
	QHostAddress selfTunIpv6;
	str nick;
	EngineState es;

public:
	EngineInterface();

	void run();

	const std::string& getNick() const;
	const QHostAddress& getTunIpv4() const;
	const QHostAddress& getTunIpv6() const;
	bool isIpv6Enabled() const;
	bool isIpv4Enabled() const;
	bool isConnected() const;
	EngineState getState() const;
	std::unordered_map<str, Network>& getNetworkMap();
	
	Network * getNetwork(const str& id);
	Peer * getPeer(const str& id);

	void netwManage(str netwId, u8, u8 unk = 0);

	void engHello();
	void engGetInfo(str lang = "en", u32 unk = 1);
	void engGetState(u8 unk = 0x93);
	void engLogin(bool autoRetryConnection = false);
	void engLogoff();

	void netwJoin(str netwId, str pass);
	void netwCreate(str netwId, str pass);
	void netwList();
	void netwConnect(str netwId);
	void netwDisconnect(str netwId);
	void netwLeave(str netwId);
	void netwDelete(str netwId);

	void engSetNick(str);

	std::shared_ptr<Promise<bool, u8>> imSendMessage(bool toNetwork, str destination, str message);
	void imSendTyping(str destination);
	
signals:
	void engineStateChanged(EngineState);
	
	void receivedPeerMessage(std::chrono::system_clock::time_point, Peer&, str msg);
	void receivedNetworkMessage(std::chrono::system_clock::time_point, Network&, Peer&, str msg);

	void peerCreated(Peer&);
	void peerUpdated(Peer&);
	void peerTyping(Peer&);
	void peerDeleted(Peer&);
	
	void networkUserCreated(Network&, const NetworkUser&);
	void networkUserUpdated(Network&, const NetworkUser&);
	void networkUserDeleted(Network&, const NetworkUser&);
	
	void networkCreated(Network&);
	void networkUpdated(Network&);
	void networkDeleted(Network&);

private:
	void hookSocketEvents();
	void hookMessageHandlers();

	template<u8 opCode, typename... Args>
	void send(Args...);

	bool processMessage(u8 opcode, u8 *data, sz_t size);

	template<u8 opCode, class F>
	void reg(F);

	void manualReg(u8 opCode, std::function<void(u8 *, sz_t)>);
};
