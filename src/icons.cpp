#include "icons.hpp"

#include <iostream>

#include <QFile>
#include <QSvgRenderer>
#include <QByteArray>
#include <QSvgRenderer>
#include <QPixmap>
#include <QPainter>

QIcon loadSvgIconReplacingColor(const char * file, QColor newColor) {
	return loadSvgIconReplacingColor(file, newColor.name().toUtf8().constData());
}

QIcon loadSvgIconReplacingColor(const char * file, const char * newColor) {
	QFile f(file);
	if (!f.open(QIODevice::ReadOnly)) {
		std::cout << "File not found! " << file << std::endl;
		return QIcon();
	}
	
	QByteArray data(f.readAll());
	data.replace("#ff00ff", newColor);
	
	QSvgRenderer svgr(data);
	if (!svgr.isValid()) {
		std::cout << "Svg not valid! " << file << std::endl;
		return QIcon();
	}
	
	QPixmap ico(svgr.defaultSize());
	ico.fill(Qt::transparent);
	QPainter qp(&ico);
	svgr.render(&qp);
	
	return QIcon(ico);
}
