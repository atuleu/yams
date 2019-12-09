set(SRC_FILES main.cpp
              MainWindow.cpp
              MediaPlayer.cpp
              )

set(HDR_FILES MainWindow.hpp
              MediaPlayer.hpp
              )

set(UI_FILES MainWindow.ui)

set(QRC_FILES )


add_executable(yams ${SRC_FILES}
                    ${HDR_FILES}
                    ${UI_FILES}
                    ${QRC_FILES})


target_link_libraries(yams Qt5::Widgets)
