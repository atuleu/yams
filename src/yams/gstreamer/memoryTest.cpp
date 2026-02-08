#include <gst/gstelement.h>
#include <gtest/gtest.h>

#include "memory.hpp"

namespace yams {
static_assert(
    sizeof(GstBufferPtr) == sizeof(GstBuffer *), "Must not have a size overhead"
);

static_assert(
    sizeof(GstElementPtr) == sizeof(GstElement *),
    "Must not have a size overhead"
);

static_assert(
    sizeof(GstVideoFramePtr) == sizeof(GstElement *),
    "Must not have a size overhead"
);

}; // namespace yams
