if(Qt6_FOUND)
    qt_add_library(QWin11PhantomStyle STATIC
        qwin11phantomcolor.cpp
        qwin11phantomstyle.cpp
        qwin11phantomcolor.h
        qwin11phantomstyle.h
        qwin11phantomtweak.h
    )
else()
    add_library(QWin11PhantomStyle STATIC
        qwin11phantomcolor.cpp
        qwin11phantomstyle.cpp
        qwin11phantomcolor.h
        qwin11phantomstyle.h
        qwin11phantomtweak.h
    )
endif()

set_target_properties(QWin11PhantomStyle PROPERTIES OUTPUT_NAME qwin11phantomstyle)
target_include_directories(QWin11PhantomStyle
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(QWin11PhantomStyle PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Gui
)