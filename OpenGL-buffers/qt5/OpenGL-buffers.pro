
QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenGL-buffers
TEMPLATE = app

SOURCES += main.cpp heightmapwidget.cpp

HEADERS += heightmapwidget.h

RESOURCES += Ressources.qrc

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -std=c++11
