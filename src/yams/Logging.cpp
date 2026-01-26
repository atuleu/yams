#include "yams/Logging.hpp"

#include <cstdlib>

#include <QtCore/QMessageLogContext>
#include <QtCore/QString>

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
		// Fatal messages should terminate the application
		slog::Fatal(message, fileAttr, lineAttr, category);
		// slog::Fatal may or may not terminate, ensure we abort
		std::abort();
		break;
	}
}

} // namespace

void initLogging() {
	// Install our custom Qt message handler
	// This must be called before QApplication is created
	qInstallMessageHandler(qtMessageHandler);
}

} // namespace yams
