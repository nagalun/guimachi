#pragma once

#include <QTranslator>

class QApplication;

class TranslationLoader {
	QTranslator qtTranslator;
	QTranslator appTranslator;
	
public:
	TranslationLoader(QApplication&);
};
