
QT += core gui
TARGET = 02-OpenGL-shader
TEMPLATE = app
CONFIG += C++14

SOURCES += main.cpp heightmapwidget.cpp

HEADERS += heightmapwidget.h

RESOURCES += Ressources.qrc

OTHER_FILES += vertex.glsl fragment.glsl
