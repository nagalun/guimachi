#pragma once

#include <QIcon>
#include <QColor>

QIcon loadSvgIconReplacingColor(const char * file, QColor newColor);
QIcon loadSvgIconReplacingColor(const char * file, const char * newColor);
