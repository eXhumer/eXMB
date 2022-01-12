configure_file(AppConfig.hxx.in AppConfig.hxx)
find_package(Qt6 COMPONENTS Network NetworkAuth Widgets REQUIRED)

qt_add_resources(
APP_RCC
    Resource/App.qrc
)

qt_wrap_cpp(
APP_MOC
    Include/AppWindow.hxx
)

add_executable(
${PROJECT_NAME}
    ${APP_MOC}
    ${APP_RCC}
    Source/AppWindow.cxx
    Source/Main.cxx
)

if(WIN32)
    target_link_libraries(
    ${PROJECT_NAME}
        PRIVATE
            dwmapi
    )

    set_target_properties(
    ${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()

target_include_directories(
${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        Include/
)

target_link_libraries(
${PROJECT_NAME}
    PUBLIC
        Qt6::NetworkAuth
        Qt6::Widgets

    PRIVATE
        eXRC
        eXVHP
)
