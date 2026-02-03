#pragma once

#include <QMetaEnum>
#include <QPoint>
#include <QRect>
#include <QSize>

#include <slog++/slog++.hpp>

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

} // namespace slog
