#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QApplication>
#include <gst/gst.h>

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);
	gst_init(&argc, &argv);
	QApplication app(argc, argv);
	return RUN_ALL_TESTS();
}
