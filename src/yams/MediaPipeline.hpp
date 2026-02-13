#pragma once

#include <chrono>
#include <slog++/slog++.hpp>

#include <QSize>

#include <yams/MediaPlayInfo.hpp>
#include <yams/gstreamer/Pipeline.hpp>

namespace yams {
class Compositor;

class MediaPipeline : public Pipeline {
	Q_OBJECT
public:
	struct Args {
		size_t LayerID = 0;
		size_t SinkID  = 0;
		QSize  Size    = {1920, 1080};
		qreal  FPS     = 60.0;
	};

	MediaPipeline(Args args, Compositor *parent);

	virtual ~MediaPipeline();
	MediaPipeline(const MediaPipeline &)            = delete;
	MediaPipeline(MediaPipeline &&)                 = delete;
	MediaPipeline &operator=(const MediaPipeline &) = delete;
	MediaPipeline &operator=(MediaPipeline &&)      = delete;

	GstElement *proxySink();

signals:
	void EOS();
	void Error();

public slots:
	void play(const MediaPlayInfo &infos);
	void stop();

protected:
	void forceDownstreamEOS();

	void onEOS();
	void onError();
	void reset();

	void playFile(const MediaPlayInfo &infos);
	void playTest(const MediaPlayInfo &infos);

	void onMessage(GstMessage *msg) noexcept override;

	slog::Logger<1> d_logger;
	GstElementPtr   d_fileSource, d_decodeBin, d_decodeCapsfilter, d_testSource,
	    d_testCapsfilter, d_timeOverlay, d_queue, d_proxySink;

	uint64_t d_framerateNum, d_framerateDenum;

	std::optional<MediaPlayInfo::Type> d_currentMedia;
	bool                               d_playing{false};
};

}; // namespace yams
