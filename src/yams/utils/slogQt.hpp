#pragma once

#include <QMetaEnum>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSurfaceFormat>

#include <iterator>
#include <qsurfaceformat.h>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>
#include <string>

namespace slog {
template <typename Str> slog::Attribute QRect(Str &&name, const QRect &r) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("x", r.x()),
	    slog::Int("y", r.y()),
	    slog::Int("width", r.width()),
	    slog::Int("height", r.height())
	);
}

template <typename Str> slog::Attribute QPoint(Str &&name, const QPoint &p) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("x", p.x()),
	    slog::Int("y", p.y())
	);
}

template <typename Str, typename Enum>
slog::Attribute QEnum(Str &&name, Enum m) {
	QMetaEnum me = QMetaEnum::fromType<Enum>();
	if (me.isValid()) {
		return slog::String(
		    std::forward<Str>(name),
		    me.valueToKey(static_cast<int>(m))
		);
	} else {
		return slog::Int(std::forward<Str>(name), static_cast<int>(m));
	}
}

template <typename Str> slog::Attribute QSize(Str &&name, const QSize &size) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("width", size.width()),
	    slog::Int("height", size.height())
	);
}

template <typename Str>
constexpr Attribute QScreen(Str &&name, const QScreen &screen) {
	return Group(
	    std::forward<Str>(name),
	    String("name", screen.name().toStdString()),
	    String("manufacturer", screen.manufacturer().toStdString()),
	    String("model", screen.model().toStdString())
	);
}

template <typename Str>
constexpr Attribute QSurfaceFormat(Str &&name, const QSurfaceFormat &fmt) {

	return Group(
	    std::forward<Str>(name),
	    QEnum("renderableType", fmt.renderableType()),
	    String(
	        "version",
	        std::to_string(fmt.majorVersion()) + "." +
	            std::to_string(fmt.minorVersion())
	    ),
	    QEnum("profile", fmt.profile()),
	    QEnum("swapBehavior", fmt.swapBehavior()),
	    String(
	        "RGBABufferSize",
	        std::to_string(fmt.redBufferSize()) + " " +
	            std::to_string(fmt.greenBufferSize()) + " " +
	            std::to_string(fmt.blueBufferSize()) + " " +
	            std::to_string(fmt.alphaBufferSize())
	    ),
	    Int("depthBufferSize", fmt.depthBufferSize()),
	    Int("stencilBufferSize", fmt.stencilBufferSize())
	);
}

} // namespace slog
