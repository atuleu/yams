option(XDESKTOP_APPSTREAM_SUPPORT
	   "Build and install the Freedesktop appstream data" OFF
)

if(XDESKTOP_APPSTREAM_SUPPORT AND NOT WIN32)
	configure_file(
		${PROJECT_SOURCE_DIR}/misc/io.github.atuleu.YAMS.desktop.in
		${PROJECT_BINARY_DIR}/misc/io.github.atuleu.YAMS.desktop @ONLY
	)

	install(FILES ${PROJECT_BINARY_DIR}/misc/io.github.atuleu.YAMS.desktop
			DESTINATION share/applications/
	)

	install(
		FILES ${PROJECT_SOURCE_DIR}/resources/icons/yams.svg
		DESTINATION share/icons/hicolor/scalable/apps
		RENAME io.github.atuleu.YAMS.svg
	)

	install(FILES ${PROJECT_SOURCE_DIR}/misc/io.github.atuleu.YAMS.metainfo.xml
			DESTINATION share/metainfo/
	)

endif()
