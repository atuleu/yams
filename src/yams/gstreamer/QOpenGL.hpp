#pragma once

#include <yams/gstreamer/Memory.hpp>
class QOpenGLContext;

namespace yams {
GstGLDisplayPtr fromGuiApplication();

GstGLContextPtr
wrapQOpenGLContext(GstGLDisplay *display, QOpenGLContext *context);
} // namespace yams
