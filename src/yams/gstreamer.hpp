#pragma once

#include <gst/gst.h>

#include <memory>

namespace yams {

template <typename T> struct GstBufferUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			gst_buffer_unref(obj);
		}
	}
};

using GstBufferPtr = std::unique_ptr<GstBuffer, GstBufferUnrefer<GstBuffer>>;

} // namespace yams
