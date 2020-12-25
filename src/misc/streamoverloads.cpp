#include "streamoverloads.hpp"

#include <QHostAddress>
#include <QString>

std::ostream& operator<<(std::ostream &o, const QString &s) {
	return o << s.toUtf8().constData();
}

std::ostream& operator<<(std::ostream &o, const QHostAddress &addr) {
    return o << addr.toString();
}
