set(SRC_FILES main.cpp
              MainWindow.cpp
              VideoWidget.cpp
              )

set(HDR_FILES MainWindow.hpp
              VideoWidget.hpp
              )

set(UI_FILES MainWindow.ui)

set(QRC_FILES )


add_executable(yams ${SRC_FILES}
                    ${HDR_FILES}
                    ${UI_FILES}
                    ${QRC_FILES})


target_link_libraries(yams Qt5::Widgets Qt5::MultimediaWidgets)
