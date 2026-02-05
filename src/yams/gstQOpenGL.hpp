#pragma once

#include "gstreamer.hpp"

class QOpenGLContext;

namespace yams {
GstGLDisplayPtr fromGuiApplication();

GstGLContextPtr
wrapQOpenGLContext(GstGLDisplay *display, QOpenGLContext *context);
} // namespace yams
