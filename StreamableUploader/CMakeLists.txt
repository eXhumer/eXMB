configure_file(AppConfig.hxx.in AppConfig.hxx)
find_package(Qt6 COMPONENTS Network NetworkAuth Widgets REQUIRED)

qt_add_resources(APP_RCC
                 Resource/App.qrc)

qt_wrap_cpp(APP_MOC
            Include/AppWindow.hxx)

if(WIN32)
add_executable(StreamableUploader
               WIN32
               ${APP_MOC}
               ${APP_RCC}
               Resource/AppLogo.ico
               Resource/Win32Version.rc
               Source/AppWindow.cxx
               Source/Main.cxx)

target_link_libraries(StreamableUploader
                      PRIVATE dwmapi)            
else()
add_executable(StreamableUploader
               ${APP_MOC}
               ${APP_RCC}
               Resource/AppLogo.ico
               Source/AppWindow.cxx
               Source/Main.cxx)
endif()

target_include_directories(StreamableUploader
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR} Include/)

target_link_libraries(StreamableUploader
                      PUBLIC Qt6::NetworkAuth Qt6::Widgets
                      PRIVATE eXVHP)

install(TARGETS StreamableUploader)
