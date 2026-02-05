#include "gstQOpenGL.hpp"

#include <QGuiApplication>
#include <QOpenGLContext>

#include "slogQt.hpp"

#include <gst/gl/gl.h>
#include <gst/gstobject.h>

#if GST_GL_HAVE_WINDOW_WAYLAND
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif

#if GST_GL_HAVE_PLATFORM_EGL
#include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if GST_GL_HAVE_WINDOW_X11
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

namespace yams {
GstGLDisplayPtr fromGuiApplication() {
	auto instance = static_cast<QGuiApplication *>(QGuiApplication::instance());
	if (QGuiApplication::platformName() == "wayland") {
		auto native =
		    instance->nativeInterface<QNativeInterface::QWaylandApplication>();
		if (native == nullptr) {
			return nullptr;
		}
		auto waylandDisplay = GstGLDisplayPtr{
		    (GstGLDisplay *)(gst_gl_display_wayland_new_with_display(
		        native->display()
		    ))
		};

		slog::Info("EGL");

		auto eglDisplay =
		    gst_gl_display_egl_from_gl_display(waylandDisplay.get());
		slog::Info("EGL done");
		gst_clear_object(&eglDisplay);

		return std::move(waylandDisplay);
	} else {
		auto native =
		    instance->nativeInterface<QNativeInterface::QX11Application>();
		if (native == nullptr) {
			return nullptr;
		}
		return GstGLDisplayPtr{
		    (GstGLDisplay *)(gst_gl_display_x11_new_with_display(
		        native->display()
		    ))
		};
	}
}

GstGLContextPtr
wrapQOpenGLContext(GstGLDisplay *display, QOpenGLContext *context) {
#ifdef Q_OS_LINUX
	if (context == nullptr) {
		return nullptr;
	}
	if (QGuiApplication::platformName() == "wayland") {
		auto native = context->nativeInterface<QNativeInterface::QEGLContext>();
		if (native == nullptr) {
			return nullptr;
		}
		return GstGLContextPtr{gst_gl_context_new_wrapped(
		    display,
		    (guintptr)native->nativeContext(),
		    GST_GL_PLATFORM_EGL,
		    GST_GL_API_OPENGL3
		)};
	} else {
		auto native = context->nativeInterface<QNativeInterface::QGLXContext>();
		if (native == nullptr) {
			return nullptr;
		}
		return GstGLContextPtr{gst_gl_context_new_wrapped(
		    display,
		    (guintptr)native->nativeContext(),
		    GST_GL_PLATFORM_GLX,
		    GST_GL_API_OPENGL3
		)};
	}
#elif Q_OS_WIN32
	return GstGLContextPtr{gst_gl_context_new_wrapped(
	    display,
	    wglGetCurrentContext(),
	    GST_GL_PLATFORM_WGL,
	    GST_GL_API_OPENGL3
	)};
#endif
}
} // namespace yams
