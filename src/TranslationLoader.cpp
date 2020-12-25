#include "TranslationLoader.hpp"

#include <QLibraryInfo>
#include <QApplication>
#include <iostream>
#include <clocale>

TranslationLoader::TranslationLoader(QApplication& app) {
	QLocale locale;
	
	std::string sysLocale(locale.name().toUtf8().constData());
	sysLocale += ".utf8"; // how is this done correctly?
	const char * result = std::setlocale(LC_ALL, sysLocale.c_str());
	
	std::cout << "Setting LC_ALL to: " << sysLocale;
	if (result) {
		std::cout << ", got: " << result << std::endl;
	} else {
		std::cout << ", failed!" << std::endl;
	}
	
	if (qtTranslator.load(locale, "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
		app.installTranslator(&qtTranslator);
	} else {
		std::cout << "QT doesn't provide translation for language: " << sysLocale << ", on: " << QLibraryInfo::location(QLibraryInfo::TranslationsPath).toUtf8().constData() << std::endl;
	}
	
	if (appTranslator.load(locale, "guimachi", "_", ":/i18n/")) {
		std::cout << "Found translation" << std::endl;
		app.installTranslator(&appTranslator);
	}
}
