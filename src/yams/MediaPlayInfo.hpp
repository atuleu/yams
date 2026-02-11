#pragma once

#include <chrono>

#include <QObject>
#include <QString>

namespace yams {

struct MediaPlayInfo {
	enum class Type {
		VIDEO,
		IMAGE,
		TEST,
	};
	MediaPlayInfo::Type      MediaType;
	QString                  Location;
	std::chrono::nanoseconds Duration;
	std::chrono::nanoseconds Fade{0};
	bool                     Loop{false};
};

} // namespace yams

Q_DECLARE_METATYPE(std::chrono::nanoseconds);
Q_DECLARE_METATYPE(yams::MediaPlayInfo);
