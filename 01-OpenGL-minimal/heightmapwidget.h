/*
    2010-2016
    Guillaume Belz
    http://guillaumebelz.wordpress.com/
    http://guillaume.belz.free.fr/

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
*/

#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <QOpenGLWindow>
#include <QOpenGLFunctions_2_0>
#include <Qtime>
#include <QVector>
#include <QVector3D>

class HeightmapWidget : public QOpenGLWindow, protected QOpenGLFunctions_2_0
{
    Q_OBJECT

public:
    explicit HeightmapWidget(QWindow* parent = nullptr);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    // OpenGL
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // Events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void rotateBy(int x, int y, int z);

private:
    // Heightmap infos
    size_t vertices_by_x{};
    size_t vertices_by_z{};
    size_t quads_by_x{};
    size_t quads_by_z{};

    // FPS
    QTime time;
    size_t frame_count{};
    size_t last_count{};

    // 3D / 2D coords
    QVector<QVector3D> m_vertices;

    // View & rotation settings
    QPoint last_pos;
    float  distance{-10.0f};
    float  x_rot{};
    float  y_rot{};
    float  z_rot{};
};

#endif // HEIGHTMAP_H
