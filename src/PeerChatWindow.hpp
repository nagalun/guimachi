#pragma once

#include "ChatWindow.hpp"

class Peer;

class PeerChatWindow : public ChatWindow {
	Q_OBJECT
	
	Peer * peer; // null if peer was deleted
	QTimer typingTimeout;
	bool isTyping;

public:
	PeerChatWindow(Peer *);
	
	using ChatWindow::receiveMessage;

	void updateStatusLabel();
	void peerTyping();
	void receiveMessage(std::chrono::system_clock::time_point time, std::string msg);
	void sendMessage(std::string);
	
	void setDestinationPtr(void *);
};
