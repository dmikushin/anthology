cmake_minimum_required(VERSION 3.1)

# Read the resulting HEX content into variable
file(READ ${MUSIC_HEX_FILE} MUSIC_HEX)

string(REGEX REPLACE "\\.[^.]*$" "" MUSIC_FILE ${MUSIC_EMBED_FILE})
get_filename_component(MUSIC_NAME ${MUSIC_FILE} NAME)
string(REGEX REPLACE "\\." "_" MUSIC_CLASS ${MUSIC_NAME})

# Substitute encoded HEX content into template source file
configure_file("${SOURCE_DIR}/src/Music.cpp.in" ${MUSIC_EMBED_FILE})

