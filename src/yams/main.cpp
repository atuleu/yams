#include <QApplication>
#include <QLabel>
#include <QMainWindow>

#include <QDebug>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	qInfo() << "Starting YAMS v0.1.0";

	QMainWindow window;
	window.setWindowTitle("Hello YAMS");

	QLabel *label = new QLabel("Hello YAMS - Phase 0.1", &window);
	window.setCentralWidget(label);
	window.resize(640, 480);
	window.show();

	qInfo() << "Main window displayed";

	return app.exec();
}
