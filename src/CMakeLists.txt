cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0076 NEW)

option(HAVE_S7 "Build with Siemens S7 support" ON)
option(HAVE_RPI "Build with Raspberry Pi IO support and CPU temperature" ON)
option(HAVE_USB "Build with libusb support (interface_K8055)" ON)
option(HAVE_MARIA "Build with libmariadbclient support" OFF)

project(tcFarmControl VERSION 1.0.0)
add_executable(tcFarmControl
	${PROJECT_BINARY_DIR}/git_header.h
)

IF(HAVE_S7)
	list(APPEND EXTRA_SRC interface_S7.cpp)
	list(APPEND EXTRA_LIBS snap7)
endif()

IF(HAVE_RPI)
	list(APPEND EXTRA_SRC interface_pi_gpio.cpp)
endif()

IF(HAVE_USB)
	list(APPEND EXTRA_SRC interface_k8055.cpp libk8055.c)
	list(APPEND EXTRA_LIBS usb)
endif()

IF(HAVE_MARIA)
	list(APPEND EXTRA_SRC interface_maria.cpp in_to_maria.cpp)
	list(APPEND EXTRA_LIBS mariadbclient)
endif()

target_link_libraries(tcFarmControl 
	PUBLIC dl pthread curl modbus jansson microhttpd stdc++fs ${EXTRA_LIBS})

configure_file(config.h.in config.h)

target_include_directories(tcFarmControl 
	PUBLIC "${PROJECT_BINARY_DIR}"
	)

set_property(TARGET tcFarmControl PROPERTY CXX_STANDARD 17)
set_property(TARGET tcFarmControl PROPERTY C_STANDARD 99)
set_property(TARGET tcFarmControl PROPERTY COMPILE_FLAGS "-W -Wall -g -O0 -Wno-psabi")

target_sources(tcFarmControl PUBLIC
	callback.c
	filters.c
	job_sched.c
	mongoose.c
	tcmath.c
	timefunc.c
	tinyexpr.c
	equation.cpp
	floatLog.cpp
	in.cpp
	in_from_dir.cpp
	in_equation.cpp
	interface_adam6052.cpp
	interface_circulac.cpp
	interface.cpp
	interface_darksky.cpp
	interface_fijnstof.cpp
	interface_fritz.cpp
	interface_host.cpp
	interface_hs110.cpp
	interface_macp.cpp
	interface_mb.cpp
	interface_sht3x.cpp
	interface_solarlog.cpp
	logic_compare.cpp
	logic.cpp
	logic_fijnstof.cpp
	logic_km.cpp
	logic_modulator.cpp
	logic_onOffRegulator.cpp
	logic_or.cpp
	logic_pi_reg.cpp
	logic_power.cpp
	logic_rain.cpp
	logic_tunnel.cpp
	main.cpp
	mytime.cpp
	out.cpp
	stringStore.cpp
	webgui.cpp
	webgui_intable.cpp
	webgui_mac.cpp
	webgui_page.cpp
	webin.cpp
	${EXTRA_SRC}
)

set_source_files_properties(interface_S1200.cpp PROPERTIES INCLUDE_DIRECTORIES "${PROJECT_BINARY_DIR}/../snap7-full-1.4.2/release/Wrappers/c-cpp/")
set_source_files_properties(interface_2B.cpp PROPERTIES INCLUDE_DIRECTORIES "${PROJECT_BINARY_DIR}/../snap7-full-1.4.2/release/Wrappers/c-cpp/")
set_source_files_properties(main.cpp PROPERTIES INCLUDE_DIRECTORIES "${PROJECT_BINARY_DIR}/../snap7-full-1.4.2/release/Wrappers/c-cpp/")

add_custom_command(
	OUTPUT 
		${PROJECT_BINARY_DIR}/__git_header.h
		${PROJECT_BINARY_DIR}/git_header.h
	COMMAND python3 ${PROJECT_SOURCE_DIR}/../build_git_header.py
)

add_custom_target(always_run ALL
	DEPENDS	${PROJECT_BINARY_DIR}/__git_header.h 
	)
