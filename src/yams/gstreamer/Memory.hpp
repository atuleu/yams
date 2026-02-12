#pragma once

#include <gst/gl/gstgl_fwd.h>
#include <gst/gst.h>

#include <gst/gstclock.h>
#include <gst/gstelementfactory.h>
#include <gst/gstobject.h>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>
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

template <typename T> struct GstVideoFrameUnmapper {
	void operator()(T *obj) const noexcept {
		if (obj == nullptr) {
			return;
		}
		gst_video_frame_unmap(obj);
		gst_object_unref(obj);
	}
};

using GstVideoFramePtr =
    std::unique_ptr<GstVideoFrame, GstVideoFrameUnmapper<GstVideoFrame>>;

template <typename T> struct GObjectUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			g_object_unref(obj);
		}
	}
};

template <typename T>
using glib_owned_ptr       = std::unique_ptr<T, GObjectUnrefer<T>>;
using GstElementPtr        = glib_owned_ptr<GstElement>;
using GstElementFactoryPtr = glib_owned_ptr<GstElementFactory>;
using GstPadPtr            = glib_owned_ptr<GstPad>;
using GstEventPtr          = glib_owned_ptr<GstEvent>;
using GstCapsPtr           = glib_owned_ptr<GstCaps>;
using GstBusPtr            = glib_owned_ptr<GstBus>;
using GstMessagePtr        = glib_owned_ptr<GstMessage>;
using GstGLDisplayPtr      = glib_owned_ptr<GstGLDisplay>;
using GstGLContextPtr      = glib_owned_ptr<GstGLContext>;
using GstClockPtr          = glib_owned_ptr<GstClock>;
} // namespace yams
