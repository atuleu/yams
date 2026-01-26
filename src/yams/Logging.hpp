#pragma once

#include <qglobal.h>

namespace yams {

/// Initialize logging infrastructure for YAMS
///
/// Installs a Qt message handler that redirects all Qt logging (qDebug, qInfo,
/// qWarning, qCritical, qFatal) to slog++ with appropriate severity level
/// mapping and structured attributes (file, line, qt_category).
///
/// @return The previously installed Qt message handler (may be nullptr if using
///         Qt's default handler). Save this value to restore the original
///         handler later using qInstallMessageHandler().
///
/// @note Must be called before QApplication is created to capture all Qt
///       messages from application startup.
///
/// @note In future phases, this will be extended to configure logging output
///       destinations (file, syslog, network) based on
///       environment/configuration.
///
/// @see qInstallMessageHandler()
///
/// Example usage:
/// @code
/// // Save original handler for later restoration
/// QtMessageHandler oldHandler = yams::initLogging();
///
/// // ... application runs with YAMS logging ...
///
/// // Restore original handler when done
/// qInstallMessageHandler(oldHandler);
/// @endcode
QtMessageHandler initLogging();

} // namespace yams
