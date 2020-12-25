#pragma once

#include <chrono>
#include <functional>

#include <QObject>
#include <QTimer>
#include <QWidget>

#include "ui_chat.h"

class QMoveEvent;
class QCloseEvent;

class ChatWindow : public QWidget, public Ui::ChatWindow {
	Q_OBJECT
	
	std::chrono::system_clock::time_point lastMessageReceived;
	std::function<void(bool)> onClose;
	bool wasMoved;
	bool newMessagesSinceHidden;
	
public:
	ChatWindow();
	
	bool isHiddenAndHasNewMessages() const;

	void receiveSystemMessage(QString, std::chrono::system_clock::time_point time = std::chrono::system_clock::now());
	void receiveMessage(QString, std::chrono::system_clock::time_point time = std::chrono::system_clock::now(), bool alert = true);
	virtual void sendMessage(std::string) = 0;
	virtual void setDestinationPtr(void *) = 0;
	
	void setCloseCb(std::function<void(bool)>);
	void clearWasMoved();
	
	void show();
	
protected:
	void moveEvent(QMoveEvent *);
	void closeEvent(QCloseEvent *);
};
