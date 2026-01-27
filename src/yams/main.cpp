#include <QApplication>
#include <QLabel>
#include <QMainWindow>

#include <slog++/slog++.hpp>

#include "Version.hpp"
#include "git.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	slog::Info(
	    "Starting YAMS",
	    slog::String("version", yams::Version()),
	    slog::String("SHA", git_CommitSHA1())
	);

	QMainWindow window;
	window.setWindowTitle("Hello YAMS");

	QLabel *label = new QLabel(
	    window.tr("Hello YAMS - Phase 0.1\nVersion: %1\nSHA: %2")
	        .arg(yams::Version().c_str(), git_CommitSHA1()),
	    &window
	);
	window.setCentralWidget(label);
	window.resize(640, 480);
	window.show();

	slog::Info("Main window displayed");

	return app.exec();
}
