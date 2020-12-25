#pragma once

#include <chrono>
#include <string>

#include "ChatWindow.hpp"

class Network;
class Peer;

class NetworkChatWindow : public ChatWindow {
	Q_OBJECT

	const Network * netw;

public:
	NetworkChatWindow(const Network *);
	
	using ChatWindow::receiveMessage;


	void receiveMessage(std::chrono::system_clock::time_point time, const Peer&, std::string);
	void sendMessage(std::string);
	
	void setDestinationPtr(void *);
};
