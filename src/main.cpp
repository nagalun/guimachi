#include "Guimachi.hpp"

#include <QCoreApplication>
#include <QTranslator>
#include <iostream>
#include <stdexcept>

int main(int argc, char ** argv) {
	try {
		return Guimachi(argc, argv).exec();
	} catch(const std::exception& e) {
		std::cout << typeid(e).name() << "::::" << e.what() << std::endl;
	} catch(...) {
		std::cout << "unk exception" << std::endl;
		throw;
	}
	
	return 1;
}
