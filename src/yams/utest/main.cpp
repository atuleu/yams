#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QApplication>
#include <gst/gst.h>

#include <slog++/slog++.hpp>

int main(int argc, char **argv) {
	slog::DefaultLogger().From(slog::Level::Debug);
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);
	gst_init(&argc, &argv);
	QApplication app(argc, argv);
	return RUN_ALL_TESTS();
}
