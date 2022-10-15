configure_file(AppConfig.hxx.in AppConfig.hxx)
find_package(Qt6 COMPONENTS Network NetworkAuth Widgets REQUIRED)

qt_add_resources(APP_RCC
                 Resource/App.qrc)

qt_wrap_cpp(APP_MOC
            Include/AppWindow.hxx)

if(WIN32)
add_executable(${PROJECT_NAME}
               WIN32
               ${APP_MOC}
               ${APP_RCC}
               Resource/AppLogo.ico
               Resource/Win32Version.rc
               Source/AppWindow.cxx
               Source/FlairListModel.cxx
               Source/Main.cxx)

target_link_libraries(${PROJECT_NAME}
                      PRIVATE dwmapi)
else()
add_executable(${PROJECT_NAME}
               ${APP_MOC}
               ${APP_RCC}
               Resource/AppLogo.ico
               Source/AppWindow.cxx
               Source/FlairListModel.cxx
               Source/Main.cxx)
endif()

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR} Include/)

target_link_libraries(${PROJECT_NAME}
                      PUBLIC Qt6::NetworkAuth Qt6::Widgets
                      PRIVATE eXRC eXVHP)

install(TARGETS ${PROJECT_NAME})
