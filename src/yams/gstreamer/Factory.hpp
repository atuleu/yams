#pragma once

#include "Memory.hpp"

#include <cpptrace/cpptrace.hpp>

namespace yams {
template <typename... Attributes>
inline GstElementPtr
GstElementFactoryMakeFull(const char *factory, Attributes &&...attrs) {
	auto res = gst_element_factory_make_full(
	    factory,
	    std::forward<Attributes>(attrs)...,
	    nullptr
	);
	if (res == nullptr) {
		throw cpptrace::runtime_error{
		    "could not make element: gst_element_factory_make_full('" +
		    std::string(factory) + "')"
		};
	}
	return GstElementPtr{res};
}
} // namespace yams
