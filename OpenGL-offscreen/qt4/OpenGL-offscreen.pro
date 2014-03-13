
QT += core gui opengl

TARGET = OpenGL-offscreen
TEMPLATE = app

SOURCES += main.cpp heightmapwidget.cpp

HEADERS += heightmapwidget.h

RESOURCES += Ressources.qrc

OTHER_FILES += vertex.glsl fragment.glsl \
    pbuffer.glsl \
    fbo.glsl
