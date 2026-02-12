#pragma once

#include <chrono>
#include <gst/gl/gstglcontext.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <memory>

#include <QObject>

namespace yams {
class Frame {
public:
	using Ptr = std::shared_ptr<Frame>;
	Frame();
	~Frame();
	Frame(const Frame &)            = delete;
	Frame &operator=(const Frame &) = delete;
	Frame(Frame &&)                 = delete;
	Frame &operator=(Frame &&)      = delete;

	bool map(GstVideoInfo *infos, GstBuffer *buffer, GstMapFlags flags);
	void unmap();

	void  waitSync(GstGLContext *);
	guint TexID();

	std::chrono::nanoseconds PTS() const;
	std::chrono::nanoseconds DTS() const;
	std::chrono::nanoseconds Duration() const;

private:
	GstVideoFrame *d_frame;
	bool           d_mapped{false};
};

} // namespace yams

Q_DECLARE_METATYPE(yams::Frame);
