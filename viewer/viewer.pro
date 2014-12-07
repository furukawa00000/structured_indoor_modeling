# Feel free to add more paths here. This should just
# give warnings without errors.

QT       += core gui widgets

TARGET = viewer
TEMPLATE = app

SOURCES += viewer.cc

QMAKE_CXXFLAGS += '-Wno-c++11-extensions -Wno-gnu-static-float-init'

qtHaveModule(opengl) {
    QT += opengl

    SOURCES += main_widget.cc \
       configuration.cc \
       navigation.cc \
       floorplan_renderer.cc \
       panorama_renderer.cc \
       polygon_renderer.cc \
       ../floorplan/floorplan.cc

    HEADERS += \
        main_widget.h \
        configuration.h \
        navigation.h \
        floorplan_renderer.h \
        panorama_renderer.h \
        polygon_renderer.h \
        ../calibration/file_io.h \
        ../floorplan/floorplan.h

    RESOURCES += \
        shaders.qrc

    INCLUDEPATH += '/Users/furukawa/Google_Drive/research/code/'
#    INCLUDEPATH += '/opt/X11/include'
     

     LIBS += '-F/Users/furukawa/Qt/5.3/clang_64/lib/QtOpenGL.framework/Versions/5/'
#    LIBS += '-L/opt/X11/lib -lGLU -framework OpenGL'
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/cube
INSTALLS += target
