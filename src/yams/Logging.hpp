#pragma once

namespace yams {

/// Initialize logging infrastructure for YAMS
///
/// This function:
/// - Installs a Qt message handler that redirects all Qt logging to slog++
/// - Maps Qt message types to appropriate slog++ severity levels
/// - Includes source location (file, line) as structured attributes
///
/// Must be called before QApplication is created to capture all Qt messages.
///
/// In future phases, this will be extended to configure logging output
/// destinations (file, syslog, network) based on environment/configuration.
void initLogging();

} // namespace yams
