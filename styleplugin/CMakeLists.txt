qt_add_library(QWin11PhantomStylePlugin SHARED
    qwin11phantomstyleplugin.h
    qwin11phantomstyleplugin.cpp
)

set_target_properties(QWin11PhantomStylePlugin PROPERTIES OUTPUT_NAME qwin11phantomstyle)
target_include_directories(QWin11PhantomStylePlugin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(QWin11PhantomStylePlugin QT_DEPRECATED_WARNINGS)
target_link_libraries(QWin11PhantomStylePlugin PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Gui
)