#include "ChatWindow.hpp"

#include <QCloseEvent>
#include <QMoveEvent>
#include <QString>
#include <QColor>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "Peer.hpp"
#include "Network.hpp"
#include "misc/colors.hpp"

ChatWindow::ChatWindow()
: lastMessageReceived(std::chrono::system_clock::now()),
  onClose([] (bool) {}),
  wasMoved(false),
  newMessagesSinceHidden(false) {
	setupUi(this);
	
	connect(chatInput, &InputBox::escapePressed, [this] {
		close();
	});
}

bool ChatWindow::isHiddenAndHasNewMessages() const {
	return newMessagesSinceHidden;
}

void ChatWindow::receiveSystemMessage(QString msg, std::chrono::system_clock::time_point time) {
	chatLog->setTextColor(QColor("lightGray"));
	receiveMessage(msg, time, false);
	chatLog->setTextColor(getTextColor());
}

void ChatWindow::receiveMessage(QString msg, std::chrono::system_clock::time_point time, bool alert) {
	auto oldtmt = std::chrono::system_clock::to_time_t(lastMessageReceived);
	auto tmt = std::chrono::system_clock::to_time_t(time);
	
	int oldTimeDay = std::localtime(&oldtmt)->tm_mday;
	std::tm * tm = std::localtime(&tmt);
	
	std::stringstream ss;
	
	lastMessageReceived = time;
	
	if (tm->tm_mday != oldTimeDay) {
		ss << '[' << tr("Date changed to: ").toUtf8().constData() << std::put_time(tm, "%x]");
		receiveSystemMessage(QString::fromStdString(ss.str()));
		ss.str(std::string());
		ss.clear();
	}
	
	ss << std::put_time(tm, "[%H:%M:%S] ");
	
	chatLog->append(QString("%1%2")
		.arg(QString::fromStdString(ss.str()), msg));
	
	if (!isActiveWindow() && alert) {
		QApplication::alert(this);
		if (isHidden()) {
			newMessagesSinceHidden = true;
		}
	}
}

void ChatWindow::setCloseCb(std::function<void(bool)> cb) {
	onClose = std::move(cb);
}

void ChatWindow::clearWasMoved() {
	wasMoved = false;
}

void ChatWindow::show() {
	newMessagesSinceHidden = false;
	QWidget::show();
}


void ChatWindow::moveEvent(QMoveEvent *) {
	wasMoved = true;
}

void ChatWindow::closeEvent(QCloseEvent * ev) {
	onClose(wasMoved);
	QWidget::closeEvent(ev);
}
