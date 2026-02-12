#include "Frame.hpp"

#include <chrono>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/gstbuffer.h>
#include <mutex>
#include <stdexcept>

namespace yams {

static std::once_flag initialized;

Frame::Frame()
    : d_frame{g_new0(GstVideoFrame, 1)} {}

Frame::~Frame() {
	g_free(d_frame);
}

bool Frame::map(GstVideoInfo *infos, GstBuffer *buffer, GstMapFlags flags) {
	d_mapped = gst_video_frame_map(d_frame, infos, buffer, flags);
	return d_mapped;
}

void Frame::unmap() {
	if (d_mapped == false) {
		return;
	}
	gst_video_frame_unmap(d_frame);
	d_mapped = false;
}

void Frame::waitSync(GstGLContext *context) {
	if (d_mapped == false) {
		throw std::runtime_error{"frame is not mapped"};
	}
	auto sync = gst_buffer_get_gl_sync_meta(d_frame->buffer);
	if (sync == nullptr) {
		throw std::runtime_error{"buffer does not contain GstGLSyncMeta"};
	}
	gst_gl_sync_meta_wait(sync, context);
}

guint Frame::TexID() {
	if (d_mapped == false) {
		return 0;
	};
	return *(guint *)(d_frame->data[0]);
}

std::chrono::nanoseconds Frame::PTS() const {
	return std::chrono::nanoseconds{GST_BUFFER_PTS(d_frame->buffer)};
}

std::chrono::nanoseconds Frame::DTS() const {
	return std::chrono::nanoseconds{GST_BUFFER_DTS(d_frame->buffer)};
}

std::chrono::nanoseconds Frame::Duration() const {
	return std::chrono::nanoseconds{GST_BUFFER_DURATION(d_frame->buffer)};
}

}; // namespace yams
