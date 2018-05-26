cmake_minimum_required(VERSION 3.1)

# Read the resulting HEX content into variable
file(READ ${GAME_HEX_FILE} GAME_HEX)

string(REGEX REPLACE "\\.[^.]*$" "" GAME_FILE ${GAME_EMBED_FILE})
get_filename_component(GAME_NAME ${GAME_FILE} NAME)
string(REGEX REPLACE "\\." "_" GAME_CLASS ${GAME_NAME})

# Substitute encoded HEX content into template source file
configure_file("${SOURCE_DIR}/src/Game.cpp.in" ${GAME_EMBED_FILE})

