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

#include "heightmapwidget.h"
#include <QMouseEvent>
#include <GL/glu.h>
#include <QMatrix4x4>
#include <QPainter>

const float MAP_SIZE = 5.0;

HeightmapWidget::HeightmapWidget(QWindow* parent) :
    QOpenGLWindow(NoPartialUpdate, parent)
{
    resize(1000, 800);
    time.start();
}

QSize HeightmapWidget::minimumSizeHint() const
{
    return QSize(200, 200);
}

QSize HeightmapWidget::sizeHint() const
{
    return QSize(800, 600);
}

void HeightmapWidget::initializeGL()
{
    // init OpenGL
    initializeOpenGLFunctions();

    // Load heightmap
    const QImage img = QImage(":/heightmap.png");

    vertices_by_x = img.width();
    vertices_by_z = img.height();
    quads_by_x = vertices_by_x - 1;
    quads_by_z = vertices_by_z - 1;

    QVector3D vertice;
    for(size_t z = 0; z < vertices_by_z; ++z)
    {
        for(size_t x = 0; x < vertices_by_x; ++x)
        {
            QRgb color = img.pixel(x, z);

            vertice.setX((MAP_SIZE * x / vertices_by_x) - MAP_SIZE / 2);
            vertice.setY(2.0 * qGray(color) / 255);
            vertice.setZ((MAP_SIZE * z / vertices_by_z) - MAP_SIZE / 2);

            m_vertices.push_back(vertice);
        }
    }

    // GL options
    glClearColor(0.52f, 0.52f, 0.52f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

// Note: old OpenGL code, use OpenGL > 2
void HeightmapWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    QMatrix4x4  modelview;
    modelview.lookAt(QVector3D(0.0f, 0.0f, -distance),
                     QVector3D(0.0f, 0.0f, 0.0f),
                     QVector3D(0.0f, 1.0f, 0.0f));
    modelview.rotate(x_rot / 16.0f, QVector3D(1.0f, 0.0f, 0.0f));
    modelview.rotate(y_rot / 16.0f, QVector3D(0.0f, 1.0f, 0.0f));
    modelview.rotate(z_rot / 16.0f, QVector3D(0.0f, 0.0f, 1.0f));
    glLoadMatrixf(modelview.constData());

    // Projection matrix
    glMatrixMode(GL_PROJECTION);
    QMatrix4x4  projection;
    projection.perspective(30.0f, 1.0f*width()/height(), 0.1f, 100.0f);
    glLoadMatrixf(projection.constData());

    // On affiche un simple repere
    glBegin(GL_LINES);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glEnd();

    glBegin(GL_LINES);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();

    glBegin(GL_LINES);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();

    // FPS count
    ++frame_count;
    const auto elapsed = time.elapsed();
    if (elapsed >= 1000)
    {
        last_count = frame_count;
        frame_count = 0;
        time.restart();
    }

    // FPS display
    glPolygonMode(GL_FRONT, GL_FILL);
    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(10.0f, 10.0f, 200.0f, 100.0f), QString("FPS:%1").arg(last_count));

    update();
}

void HeightmapWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void HeightmapWidget::mousePressEvent(QMouseEvent *event)
{
    last_pos = event->pos();
}

void HeightmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto dx = event->x() - last_pos.x();
    const auto dy = event->y() - last_pos.y();

    if (event->buttons())
    {
        rotateBy(dy*8, 0, 0);
        rotateBy(0, dx*8, 0);
    }
    last_pos = event->pos();
}

void HeightmapWidget::wheelEvent(QWheelEvent *event)
{
    distance *= 1.0f + (1.0f * event->delta() / 1200.0f);
}

void HeightmapWidget::rotateBy(int x, int y, int z)
{
    x_rot += x;
    y_rot += y;
    z_rot += z;
}
