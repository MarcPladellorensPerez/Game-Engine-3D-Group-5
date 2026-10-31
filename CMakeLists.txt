cmake_minimum_required(VERSION 3.10)

project(MotorVJ CXX)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(TIFF QUIET)
find_package(PNG QUIET)
find_package(JPEG QUIET)
find_package(ZLIB QUIET)
find_package(OpenEXR CONFIG QUIET)
find_library(JASPER_LIBRARY NAMES jasper jasperd)
find_library(LCMS2_LIBRARY NAMES lcms2 lcms2d)
find_path(STB_INCLUDE_DIR stb_image.h REQUIRED)

file(GLOB_RECURSE SOURCES "Source/*.cpp")
file(GLOB_RECURSE HEADERS "Source/*.h")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(${PROJECT_NAME} PRIVATE Source ${STB_INCLUDE_DIR})
if (DEFINED IL_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${IL_INCLUDE_DIR})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
if (TARGET DevIL::IL)
    target_link_libraries(${PROJECT_NAME} PRIVATE DevIL::IL DevIL::ILU)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${IL_LIBRARIES} ${ILU_LIBRARIES})
endif()
if (TARGET TIFF::TIFF)
    target_link_libraries(${PROJECT_NAME} PRIVATE TIFF::TIFF)
endif()
if (TARGET PNG::PNG)
    target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)
endif()
if (TARGET JPEG::JPEG)
    target_link_libraries(${PROJECT_NAME} PRIVATE JPEG::JPEG)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE jpeg)
endif()
if (TARGET ZLIB::ZLIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
endif()
if (TARGET OpenEXR::OpenEXR)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenEXR::OpenEXR)
endif()
if (JASPER_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${JASPER_LIBRARY})
    if (TARGET JPEG::JPEG)
        target_link_libraries(${PROJECT_NAME} PRIVATE JPEG::JPEG)
    endif()
endif()
if (LCMS2_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LCMS2_LIBRARY})
endif()
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()

# Copy assets to executable directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
