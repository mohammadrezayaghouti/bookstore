cmake_minimum_required(VERSION 3.16)

project(bookstore VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(bookstore
    # MANUAL_FINALIZATION # Android Only
    main.cpp login.cpp
    login.h login.ui
    registerwindow.h registerwindow.cpp
    home.h home.cpp
    addbook.h addbook.cpp
    userprofile.h userprofile.cpp
    mybooks.h mybooks.cpp
    mypurchases.h mypurchases.cpp
    cart.h cart.cpp
    networkmanager.h networkmanager.cpp
    icons.qrc
    styles.qrc

)

target_link_libraries(bookstore PRIVATE Qt6::Widgets Qt6::Network)

set_target_properties(bookstore PROPERTIES WIN32_EXECUTABLE TRUE)

include(GNUInstallDirs)
install(TARGETS bookstore
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Android Only
# qt_finalize_executable(bookstore)
