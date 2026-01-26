#include <QApplication>
#include <QLabel>
#include <QMainWindow>

#include <slog++/slog++.hpp>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	slog::Info("Starting YAMS", slog::String("version", "v0.1.0"));

	QMainWindow window;
	window.setWindowTitle("Hello YAMS");

	QLabel *label = new QLabel("Hello YAMS - Phase 0.1", &window);
	window.setCentralWidget(label);
	window.resize(640, 480);
	window.show();

	slog::Info("Main window displayed");

	return app.exec();
}
