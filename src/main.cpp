#include "MainWindow.hpp"

#include <QApplication>

int main(int argc, char ** argv) {
	QCoreApplication::setOrganizationName("Alexandre Tuleu");
	QCoreApplication::setOrganizationDomain("atuleu.github.io");
	QCoreApplication::setApplicationName("yams");
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QApplication yams(argc,argv);

	MainWindow window;
	window.show();

	return yams.exec();
}
