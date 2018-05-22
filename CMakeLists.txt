cmake_minimum_required(VERSION 3.1)

project(anthology)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

# Search path for CMake include files.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)
find_package(SDL REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Manual build type selection (for debugging purposes)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

# Enable C++ 11 support
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE BINARY_SRC "src/*.c" "src/*.cpp")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/debugger)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ui/gtk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ui/scaler)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/peripherals)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/peripherals/disk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/peripherals/flash)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/peripherals/ide)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/peripherals/nic)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/machines)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pokefinder)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/sound)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/timer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/unittests)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/z80)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/gio-unix-2.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cairo)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pango-1.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/atk-1.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/cairo)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pixman-1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/libpng12)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/gdk-pixbuf-2.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/libpng12)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pango-1.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/harfbuzz)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pango-1.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/freetype2)
include_directories(${SDL_INCLUDE_DIR})
include_directories(${GTK3_INCLUDE_DIRS})

link_directories(${GTK3_LIBRARY_DIRS})

add_executable(${PROJECT_NAME}_xxd ${CMAKE_CURRENT_SOURCE_DIR}/xxd/xxd.c)

# Embed music.
file(GLOB MUSIC_SRC "music/*.ogg")
set(MUSIC_EMBED ${PROJECT_NAME}_xxd)
set(MUSIC_INTERMEDIATE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/music)
foreach(MUSIC_FILE ${MUSIC_SRC})
	# Translate music tracks into comma-separated byte codes.
	get_filename_component(MUSIC_FILE_BASE ${MUSIC_FILE} NAME)
	set(MUSIC_HEX_FILE "${MUSIC_INTERMEDIATE_DIRECTORY}/${MUSIC_FILE_BASE}.hex")
    add_custom_command(
        OUTPUT ${MUSIC_HEX_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MUSIC_INTERMEDIATE_DIRECTORY}"
        COMMAND ${MUSIC_EMBED} -i < ${MUSIC_FILE} > ${MUSIC_HEX_FILE}
        COMMENT "Generating hex representation for music file ${MUSIC_FILE}"
        DEPENDS ${MUSIC_FILE} ${PROJECT_NAME}_xxd)
	set_source_files_properties("${MUSIC_HEX_FILE}" PROPERTIES GENERATED TRUE) 
	set(MUSIC_EMBED_FILE "${MUSIC_INTERMEDIATE_DIRECTORY}/${MUSIC_FILE_BASE}.cpp")
	add_custom_command(
		OUTPUT ${MUSIC_EMBED_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MUSIC_INTERMEDIATE_DIRECTORY}"
		COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DMUSIC_HEX_FILE=${MUSIC_HEX_FILE} -DMUSIC_EMBED_FILE=${MUSIC_EMBED_FILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateMusic.cmake
		COMMENT "Embedding music file ${MUSIC_FILE}"
		DEPENDS ${MUSIC_HEX_FILE} "${CMAKE_SOURCE_DIR}/src/Music.cpp.in" "${CMAKE_SOURCE_DIR}/cmake/GenerateMusic.cmake")
	set_source_files_properties("${MUSIC_EMBED_FILE}" PROPERTIES GENERATED TRUE) 
	# Submit the resulting source file for compilation
    LIST(APPEND BINARY_SRC ${MUSIC_EMBED_FILE})
endforeach()

# Embed images.
file(GLOB_RECURSE IMAGE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/games" "*.png")
set(IMAGE_EMBED ${PROJECT_NAME}_xxd)
set(IMAGE_INTERMEDIATE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images)
foreach(IMAGE_FILE ${IMAGE_SRC})
	# Translate image sources into comma-separated byte codes.
	get_filename_component(IMAGE_FILE_BASE ${IMAGE_FILE} NAME)
	set(IMAGE_HEX_FILE "${IMAGE_INTERMEDIATE_DIRECTORY}/${IMAGE_FILE_BASE}.hex")
    add_custom_command(
        OUTPUT ${IMAGE_HEX_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IMAGE_INTERMEDIATE_DIRECTORY}"
        COMMAND ${IMAGE_EMBED} -i < ${IMAGE_FILE} > ${IMAGE_HEX_FILE}
        COMMENT "Generating hex representation for image file ${IMAGE_FILE}"
        DEPENDS ${IMAGE_FILE} ${PROJECT_NAME}_xxd)
	set_source_files_properties("${IMAGE_HEX_FILE}" PROPERTIES GENERATED TRUE) 
	set(IMAGE_EMBED_FILE "${IMAGE_INTERMEDIATE_DIRECTORY}/${IMAGE_FILE_BASE}.cpp")
	add_custom_command(
		OUTPUT ${IMAGE_EMBED_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IMAGE_INTERMEDIATE_DIRECTORY}"
		COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DIMAGE_HEX_FILE=${IMAGE_HEX_FILE} -DIMAGE_EMBED_FILE=${IMAGE_EMBED_FILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateImage.cmake
		COMMENT "Embedding image file ${IMAGE_FILE}"
		DEPENDS ${IMAGE_HEX_FILE} "${CMAKE_SOURCE_DIR}/src/Image.cpp.in" "${CMAKE_SOURCE_DIR}/cmake/GenerateImage.cmake")
	set_source_files_properties("${IMAGE_EMBED_FILE}" PROPERTIES GENERATED TRUE) 
	# Submit the resulting source file for compilation
    LIST(APPEND BINARY_SRC ${IMAGE_EMBED_FILE})
endforeach()

# Embed games.
file(GLOB_RECURSE GAME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/games" "*.tzx")
set(GAME_EMBED ${PROJECT_NAME}_xxd)
set(GAME_INTERMEDIATE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images)
foreach(GAME_FILE ${GAME_SRC})
	# Translate game sources into comma-separated byte codes.
	get_filename_component(GAME_FILE_BASE ${GAME_FILE} NAME)
	set(GAME_HEX_FILE "${GAME_INTERMEDIATE_DIRECTORY}/${GAME_FILE_BASE}.hex")
    add_custom_command(
        OUTPUT ${GAME_HEX_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_INTERMEDIATE_DIRECTORY}"
        COMMAND ${GAME_EMBED} -i < ${GAME_FILE} > ${GAME_HEX_FILE}
        COMMENT "Generating hex representation for game file ${GAME_FILE}"
        DEPENDS ${GAME_FILE} ${PROJECT_NAME}_xxd)
	set_source_files_properties("${GAME_HEX_FILE}" PROPERTIES GENERATED TRUE) 
	set(GAME_EMBED_FILE "${GAME_INTERMEDIATE_DIRECTORY}/${GAME_FILE_BASE}.cpp")
	add_custom_command(
		OUTPUT ${GAME_EMBED_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_INTERMEDIATE_DIRECTORY}"
		COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DGAME_HEX_FILE=${GAME_HEX_FILE} -DGAME_EMBED_FILE=${GAME_EMBED_FILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateGame.cmake
		COMMENT "Embedding game file ${GAME_FILE}"
		DEPENDS ${GAME_HEX_FILE} "${CMAKE_SOURCE_DIR}/src/Game.cpp.in" "${CMAKE_SOURCE_DIR}/cmake/GenerateGame.cmake")
	set_source_files_properties("${GAME_EMBED_FILE}" PROPERTIES GENERATED TRUE) 
	# Submit the resulting source file for compilation
    LIST(APPEND BINARY_SRC ${GAME_EMBED_FILE})
endforeach()

add_executable(${PROJECT_NAME} ${BINARY_SRC})
target_link_libraries(${PROJECT_NAME} glib-2.0 spectrum png12 asound ${SDL_LIBRARY} z m rt ${GTK3_LIBRARIES} X11 allegro_audio allegro_acodec allegro_memfile allegro)

target_compile_definitions(${PROJECT_NAME} PUBLIC HAVE_CONFIG_H)
target_compile_definitions(${PROJECT_NAME} PUBLIC FUSEDATADIR="/usr/share/anthology")
target_compile_definitions(${PROJECT_NAME} PUBLIC _GNU_SOURCE=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC _REENTRANT)
add_definitions(${GTK3_CFLAGS_OTHER})

