set(SRC_FILES main.cpp
              MainWindow.cpp
              VideoWidget.cpp
              PlayerControl.cpp
              PlaylistModel.cpp
              )

set(HDR_FILES MainWindow.hpp
              VideoWidget.hpp
              PlayerControl.hpp
              PlaylistModel.hpp
              )

set(UI_FILES MainWindow.ui
             PlayerControl.ui)

set(QRC_FILES )

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(yams ${SRC_FILES}
                    ${HDR_FILES}
                    ${UI_FILES}
                    ${QRC_FILES})


target_link_libraries(yams Qt5::Widgets Qt5::MultimediaWidgets)
