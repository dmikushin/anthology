cmake_minimum_required(VERSION 3.1)

# Read the resulting HEX content into variable
file(READ ${IMAGE_HEX_FILE} IMAGE_HEX)

string(REGEX REPLACE "\\.[^.]*$" "" IMAGE_FILE ${IMAGE_EMBED_FILE})
get_filename_component(IMAGE_NAME ${IMAGE_FILE} NAME)
string(REGEX REPLACE "\\." "_" IMAGE_CLASS ${IMAGE_NAME})

# Substitute encoded HEX content into template source file
configure_file("${SOURCE_DIR}/src/Image.cpp.in" ${IMAGE_EMBED_FILE})

