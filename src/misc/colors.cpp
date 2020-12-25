#include "colors.hpp"

#include <QApplication>
#include <QPalette>
#include <QColor>

QColor getTextColor() {
	return QApplication::palette().text().color();
}
