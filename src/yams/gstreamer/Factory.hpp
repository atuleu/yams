#pragma once

#include "Memory.hpp"

#include <cpptrace/cpptrace.hpp>

namespace yams {
inline GstElementPtr
GstElementFactoryMake(const char *factory, const char *name) {
	auto res = gst_element_factory_make(factory, name);
	if (res == nullptr) {
		throw cpptrace::runtime_error{
		    "could not make element: gst_element_factory_make('" +
		    std::string(factory) + "','" + std::string{name} + "')"
		};
	}
	return GstElementPtr{res};
}
} // namespace yams
