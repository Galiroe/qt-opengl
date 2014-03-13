
QT += core gui opengl

TARGET = OpenGL-shader
TEMPLATE = app

SOURCES += main.cpp heightmapwidget.cpp

HEADERS += heightmapwidget.h

RESOURCES += Ressources.qrc

OTHER_FILES += vertex.glsl fragment.glsl
