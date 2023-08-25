# main CMAKE file for the xylobit project

# required cmake stuff
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xylobit)


set(COMPONENTS
	"xylobit_speaker"
	"xylobit_keyboard"
	"xylobit_control"
	"xylobit_lcd"
	"xylobit_storage"
	"xylobit_record"
	"xylobit_uti"
	"xylobit_apwifi"
	"xylobit_website"
	)
