/*
    2010-2014
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

const float MAP_SIZE = 5.0;
QString MODE_NAME[4] = { "MODE_GL_VERTEX",
                         "MODE_VERTEXARRAY",
                         "MODE_VERTEXARRAY_INDICES",
                         "MODE_VERTEBUFFEROBJECT_INDICES" };

HeightmapWidget::HeightmapWidget(QWidget *parent) :
        QGLWidget(parent),
        m_vertexbuffer(QGLBuffer::VertexBuffer),
        m_indicebuffer(QGLBuffer::IndexBuffer),
        m_texturebuffer(QGLBuffer::VertexBuffer)
{
    // Render mode set up
    mode_rendu = MODE_GL_VERTEX;
    mode_texture = true;
    mode_fill = true;

    // Timer settings
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    delay = 20;
    timer.start(delay);
    frame_count = 0;
    last_count = 0;
    last_time = QTime::currentTime();
}

HeightmapWidget::~HeightmapWidget()
{
    deleteTexture(m_textureid);
}

QSize HeightmapWidget::minimumSizeHint() const
{
    return QSize(200,200);
}

QSize HeightmapWidget::sizeHint() const
{
    return QSize(800,600);
}

void HeightmapWidget::initializeGL()
{
    // View & rotation settings
    distance = -10.0;
    x_rot = 0;
    y_rot = 0;
    z_rot = 0;

    // Load heightmap
    QImage img = QImage(":/heightmap.png");

    vertices_by_x = img.width();
    vertices_by_z = img.height();
    quads_by_x = vertices_by_x - 1;
    quads_by_z = vertices_by_z - 1;

    QVector3D vertice;
    QVector2D texture;
    m_vertices.reserve(vertices_by_x * vertices_by_z);
    m_textures.reserve(vertices_by_x * vertices_by_z);
    for(int z = 0; z < vertices_by_z; ++z)
    {
        for(int x = 0; x < vertices_by_x; ++x)
        {
            QRgb color = img.pixel(x, z);

            vertice.setX((MAP_SIZE * x / vertices_by_x) - MAP_SIZE / 2);
            vertice.setY(2.0 * qGray(color) / 255);
            vertice.setZ((MAP_SIZE * z / vertices_by_z) - MAP_SIZE / 2);
            m_vertices.push_back(vertice);

            texture.setX(static_cast<float>(x) / static_cast<float>(vertices_by_x));
            texture.setY(1.0 - static_cast<float>(z) / static_cast<float>(vertices_by_z));
            m_textures.push_back(texture);
        }
    }

    // Vertex array & indices
    m_vertexarray.reserve(quads_by_x * quads_by_z * 6);
    m_texturearray.reserve(quads_by_x * quads_by_z * 6);
    m_indices.reserve(quads_by_x * quads_by_z * 6);
    for (int z = 0; z < quads_by_z; ++z)
    {
        for (int x = 0; x < quads_by_x; ++x)
        {
            int i = z * vertices_by_x + x;

            // VertexArray
            m_vertexarray.push_back(m_vertices[i]);
            m_vertexarray.push_back(m_vertices[i+vertices_by_x]);
            m_vertexarray.push_back(m_vertices[i+1]);

            m_vertexarray.push_back(m_vertices[i+1]);
            m_vertexarray.push_back(m_vertices[i+vertices_by_x]);
            m_vertexarray.push_back(m_vertices[i+1+vertices_by_x]);

            // Texture array
            m_texturearray.push_back(m_textures[i]);
            m_texturearray.push_back(m_textures[i+vertices_by_x]);
            m_texturearray.push_back(m_textures[i+1]);

            m_texturearray.push_back(m_textures[i+1]);
            m_texturearray.push_back(m_textures[i+vertices_by_x]);
            m_texturearray.push_back(m_textures[i+1+vertices_by_x]);

            // Indices
            m_indices.push_back(i);
            m_indices.push_back(i + vertices_by_x);
            m_indices.push_back(i + 1);

            m_indices.push_back(i + 1);
            m_indices.push_back(i + vertices_by_x);
            m_indices.push_back(i + 1 + vertices_by_x);
        }
    }

    // Load texture
    m_textureid = bindTexture(QPixmap(":/texture.png"), GL_TEXTURE_2D);

    // Vertex buffer init
    m_vertexbuffer.create();
    m_vertexbuffer.bind();
    m_vertexbuffer.allocate(m_vertices.constData(), m_vertices.size() * sizeof(QVector3D));
    m_vertexbuffer.release();

    // Texture coordonnees vbo
    m_texturebuffer.create();
    m_texturebuffer.bind();
    m_texturebuffer.allocate(m_textures.constData(), sizeof(QVector2D) * m_textures.size());
    m_texturebuffer.release();

    // Indices buffer init
    m_indicebuffer.create();
    m_indicebuffer.bind();
    m_indicebuffer.allocate(m_indices.constData(), m_indices.size() * sizeof(GLuint));
    m_indicebuffer.release();

    // GL options
    qglClearColor(Qt::darkGray);
    glEnable(GL_DEPTH_TEST);
}

void HeightmapWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT, mode_fill ? GL_FILL : GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);

    if (mode_texture)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);

    // Model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, -distance,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);

    glRotatef(x_rot / 16.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(y_rot / 16.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(z_rot / 16.0f, 0.0f, 0.0f, 1.0f);

    // Projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30.0f, 1.0*width()/height(), 0.1f, 100.0f);

    // Draw map
    qglColor(Qt::white);

    switch(mode_rendu)
    {
    case MODE_GL_VERTEX:        
        glBegin(GL_TRIANGLES);
        for(int i = 0; i < m_vertexarray.size(); i += 3)
        {

            glBindTexture(GL_TEXTURE_2D, m_textureid);

            glTexCoord2f(m_texturearray[i].x(), m_texturearray[i].y());
            glVertex3f(m_vertexarray[i].x(), m_vertexarray[i].y(), m_vertexarray[i].z());

            glTexCoord2f(m_texturearray[i+1].x(), m_texturearray[i+1].y());
            glVertex3f(m_vertexarray[i+1].x(), m_vertexarray[i+1].y(), m_vertexarray[i+1].z());

            glTexCoord2f(m_texturearray[i+2].x(), m_texturearray[i+2].y());
            glVertex3f(m_vertexarray[i+2].x(), m_vertexarray[i+2].y(), m_vertexarray[i+2].z());
        }        
        glEnd();
        break;

    case MODE_VERTEXARRAY:
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, m_vertexarray.constData());
        glTexCoordPointer(2, GL_FLOAT, 0, m_texturearray.constData());

        glDrawArrays(GL_TRIANGLES, 0, m_vertexarray.size());

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        break;

    case MODE_VERTEXARRAY_INDICES:
        glEnableClientState(GL_VERTEX_ARRAY);        
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, m_vertices.constData());
        glTexCoordPointer(2, GL_FLOAT, 0, m_textures.constData());

        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, m_indices.constData());

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        break;

    case MODE_VERTEBUFFEROBJECT_INDICES:
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        m_vertexbuffer.bind();
        glVertexPointer(3, GL_FLOAT, 0, NULL);
        m_vertexbuffer.release();

        m_texturebuffer.bind();
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);
        m_texturebuffer.release();

        m_indicebuffer.bind();
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
        m_indicebuffer.release();

        glDisableClientState(GL_VERTEX_ARRAY);
        break;
    }

    // FPS count
    ++frame_count;
    QTime new_time = QTime::currentTime();
    // if 1 sec (1000 msec) ellapsed
    if (last_time.msecsTo(new_time) >= 1000)
    {
        last_count = frame_count;
        frame_count = 0;
        last_time = QTime::currentTime();
    }

    // Debug display
    qglColor(Qt::white);
    renderText(10, 15, "Press spacebar to change transfert mode");
    renderText(10, 30, "Press T to draw on/off texture");
    renderText(10, 45, "Press F to fill on/off");
    renderText(10, 60, "Press Up/Down to increase/decrease delay");
    renderText(10, height()-55, "Delay : " + QString::number(delay));
    renderText(10, height()-40, "Size points : " + QString::number(m_indices.size()));
    renderText(10, height()-25, QString("%1 : %2").arg(MODE_NAME[mode_rendu]).arg(last_count));
    renderText(10, height()-10, QString("Rotation x:%1 y:%2 z:%3").arg(x_rot/16).arg(y_rot/16).arg(z_rot/16));
}

void HeightmapWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void HeightmapWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space)
        mode_rendu  = static_cast<MODE_RENDU>((mode_rendu + 1) % 4);

    if (event->key() == Qt::Key_T)
        mode_texture = !mode_texture;

    if (event->key() == Qt::Key_F)
        mode_fill = !mode_fill;

    if (event->key() == Qt::Key_Up)
    {
        if (delay < 50) ++delay;
        timer.start(delay);
    }

    if (event->key() == Qt::Key_Down)
    {
        if (delay > 0) --delay;
        timer.start(delay);
    }
}

void HeightmapWidget::mousePressEvent(QMouseEvent *event)
{
    last_pos = event->pos();
}

void HeightmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - last_pos.x();
    int dy = event->y() - last_pos.y();

    if (event->buttons() & Qt::RightButton)
    {
        rotateBy(dy*8, 0, 0);
        rotateBy(0, dx*8, 0);
    }
    last_pos = event->pos();
}

void HeightmapWidget::wheelEvent(QWheelEvent *event)
{
    distance *= 1.0 + (1.0 * event->delta() / 1200.0);
}

void HeightmapWidget::rotateBy(int x, int y, int z)
{
    x_rot += x;
    y_rot += y;
    z_rot += z;
}
