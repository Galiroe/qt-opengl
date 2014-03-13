
QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenGL-shader
TEMPLATE = app

SOURCES += main.cpp heightmapwidget.cpp

HEADERS += heightmapwidget.h

RESOURCES += Ressources.qrc

OTHER_FILES += vertex.glsl fragment.glsl

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -std=c++11
