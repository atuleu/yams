#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QApplication>

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);

	QApplication app(argc, argv);
	return RUN_ALL_TESTS();
}
