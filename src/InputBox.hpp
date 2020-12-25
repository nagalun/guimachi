#pragma once

#include <QPlainTextEdit>

class QKeyEvent;
class QWidget;

class InputBox : public QPlainTextEdit {
	Q_OBJECT
	
public:
	InputBox(QWidget *);
	
	void keyPressEvent(QKeyEvent *);
	
signals:
	void sendContents();
	void escapePressed();
};
