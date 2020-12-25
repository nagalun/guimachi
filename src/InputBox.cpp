#include "InputBox.hpp"

#include <QKeyEvent>
#include <QWidget>

InputBox::InputBox(QWidget * parent)
: QPlainTextEdit(parent) { }

void InputBox::keyPressEvent(QKeyEvent * ev) {
	switch (ev->key()) {
		case Qt::Key_Escape:
			emit escapePressed();
			break;
			
		case Qt::Key_Return:
			if (!(ev->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))) {
				emit sendContents();
				break;
			}
			[[fallthrough]];
		default:
			QPlainTextEdit::keyPressEvent(ev);
			break;
	}
}
