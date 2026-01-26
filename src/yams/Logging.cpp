#include "yams/Logging.hpp"

#include <cstdlib>

#include <QtCore/QMessageLogContext>
#include <QtCore/QString>

#include <qglobal.h>
#include <slog++/slog++.hpp>

namespace yams {

namespace {

/// Qt message handler that redirects all Qt logging to slog++
///
/// Maps Qt message types to slog++ severity levels and includes
/// source location as structured attributes.
void qtMessageHandler(
    QtMsgType type, const QMessageLogContext &context, const QString &msg
) {
	// Convert QString to std::string
	std::string message = msg.toStdString();

	// Prepare structured attributes for source location
	// Only include file/line if available (Qt may not always provide them)
	auto fileAttr = context.file ? slog::String("file", context.file)
	                             : slog::String("file", "");
	auto lineAttr = context.line > 0 ? slog::Int("line", context.line)
	                                 : slog::Int("line", 0);

	auto category = context.category
	                    ? slog::String("qt_category", context.category)
	                    : slog::String("qt_category", "");

	// Map Qt message type to slog++ level and log
	switch (type) {
	case QtDebugMsg:
		slog::Debug(message, fileAttr, lineAttr, category);
		break;

	case QtInfoMsg:
		slog::Info(message, fileAttr, lineAttr, category);
		break;

	case QtWarningMsg:
		slog::Warn(message, fileAttr, lineAttr, category);
		break;

	case QtCriticalMsg:
		slog::Error(message, fileAttr, lineAttr, category);
		break;

	case QtFatalMsg:
		// slog will terminate.
		slog::Fatal(message, fileAttr, lineAttr, category);
		break;
	}
}

} // namespace

QtMessageHandler initLogging() {
	// Install our custom Qt message handler and return the previous one
	// This must be called before QApplication is created to capture all
	// messages
	return qInstallMessageHandler(qtMessageHandler);
}

} // namespace yams
