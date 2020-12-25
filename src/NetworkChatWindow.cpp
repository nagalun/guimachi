#include "NetworkChatWindow.hpp"

#include "Network.hpp"

#include <iostream>

NetworkChatWindow::NetworkChatWindow(const Network * netw)
: netw(netw) {
	connect(chatInput, &InputBox::sendContents, [this] {
		sendMessage(chatInput->toPlainText().toUtf8().constData());
	});
	
	statusLabel->setHidden(true);
	setWindowTitle(QString("%1 (%2)")
		.arg(netw->getName().c_str(), netw->getId().c_str()));
}

void NetworkChatWindow::receiveMessage(std::chrono::system_clock::time_point time, const Peer& p, std::string msg) {
	ChatWindow::receiveMessage(QString("%1: %2")
		.arg(QString::fromStdString(p.getNick()), QString::fromStdString(msg)), time);
}


void NetworkChatWindow::sendMessage(std::string str) {
	if (!str.size() || !netw) {
		return;
	}
	
	chatInput->setReadOnly(true);
	netw->sendMessage(std::move(str))->then([this] (bool) {
		std::cout << "Sent message to network" << std::endl;
		ChatWindow::receiveMessage("→ " + chatInput->toPlainText());
		chatInput->setReadOnly(false);
		chatInput->clear();
		
	}, [this] (u8 errcode) {
		std::cout << "Failed to send message to network " << errcode << std::endl;
		chatInput->setReadOnly(false);
	});
}

void NetworkChatWindow::setDestinationPtr(void * ptr) {
	netw = static_cast<Network *>(ptr);
}
