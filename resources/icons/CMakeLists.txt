set(ICONS_SVG_FILES yams.svg #
					yams-uninstall.svg
)

set(ICONS_RESOLUTIONS
	16
	24
	32
	48
	64
	128
	192
	256
)

set(ICONS_OUTPUTS "")
foreach(i ${ICONS_SVG_FILES})
	string(REPLACE ".svg" "" base ${i})
	set(resfiles "")
	foreach(r ${ICONS_RESOLUTIONS})
		set(resfile ${CMAKE_CURRENT_SOURCE_DIR}/${base}-${r}.png)
		add_custom_command(
			OUTPUT ${resfile}
			COMMAND
				flatpak run org.inkscape.Inkscape -h ${r} -w ${r} -o
				${CMAKE_CURRENT_SOURCE_DIR}/${base}-${r}.png
				${CMAKE_CURRENT_SOURCE_DIR}/${i}
			DEPENDS ${i}
			COMMENT "Generating ${base}-${r}.png"
		)
		list(APPEND resfiles ${resfile})
	endforeach()
	set(icofile ${CMAKE_CURRENT_SOURCE_DIR}/${base}.ico)
	add_custom_command(
		OUTPUT ${icofile}
		COMMAND convert ${resfiles} ${icofile}
		DEPENDS ${resfiles}
		COMMENT "Generating ${base}.ico"
	)
	list(APPEND ICONS_OUTPUTS ${icofile})
endforeach()

add_custom_target(generate-icons DEPENDS ${ICONS_OUTPUTS})
