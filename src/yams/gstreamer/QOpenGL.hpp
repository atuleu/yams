#pragma once

#include <yams/gstreamer/memory.hpp>
class QOpenGLContext;

namespace yams {
GstGLDisplayPtr fromGuiApplication();

GstGLContextPtr
wrapQOpenGLContext(GstGLDisplay *display, QOpenGLContext *context);
} // namespace yams
