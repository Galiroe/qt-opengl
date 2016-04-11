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
#include <QtGui/QMouseEvent>
#include <math.h>

const float MAP_SIZE = 5.0;
enum { PROGRAM_VERTEX_ATTRIBUTE, PROGRAM_NORMAL_ATTRIBUTE, PROGRAM_TEXTURE_ATTRIBUTE };
QString MODE_NAME[5] = { "MODE_IMAGE",
                         "MODE_PBUFFER",
                         "MODE_FBO",
                         "MODE_PBUFFER_PAINTER",
                         "MODE_FBO_PAINTER" };

HeightmapWidget::HeightmapWidget(QWidget *parent) :
        QGLWidget(parent),
        m_vertexbuffer(QGLBuffer::VertexBuffer),
        m_normalbuffer(QGLBuffer::VertexBuffer),
        m_indicebuffer(QGLBuffer::IndexBuffer),
        m_texturebuffer(QGLBuffer::VertexBuffer),
        m_pbuffer(NULL), m_fbo(NULL)
{
    // Timer settings
    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
    timer.start(20);
    frame_count = 0;
    last_count = 0;
    last_time = QTime::currentTime();

    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);

    mode_rendu = MODE_IMAGE;
}

HeightmapWidget::~HeightmapWidget()
{
    deleteTexture(m_textureid);

    if (m_pbuffer)
    {
        m_pbuffer->releaseFromDynamicTexture();
        glDeleteTextures(1, &m_pbuffer_location);
        delete m_pbuffer;
    }

    if (m_fbo) delete m_fbo;
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

    // Vertices & texture
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

            texture.setX(1.0 * x / quads_by_x);
            texture.setY(1.0 - 1.0 * z / quads_by_z);
            m_textures.push_back(texture);
        }
    }

    // Indices
    m_indices.reserve(quads_by_x * quads_by_z * 6);
    for (int z = 0; z < quads_by_z; ++z)
    {
        for (int x = 0; x < quads_by_x; ++x)
        {
            int i = z * vertices_by_x + x;

            // Indices
            m_indices.push_back(i);
            m_indices.push_back(i + vertices_by_x);
            m_indices.push_back(i + 1);

            m_indices.push_back(i + 1);
            m_indices.push_back(i + vertices_by_x);
            m_indices.push_back(i + 1 + vertices_by_x);
        }
    }

    // Normals
    img = QImage(":/normals.png");
    m_normals.reserve(m_indices.size());
    for(int z = 0; z < vertices_by_z; ++z)
    {
        for(int x = 0; x < vertices_by_x; ++x)
        {
            QVector3D normal;
            QRgb rgb = img.pixel(x, z);
            normal.setX(1.0 * qRed(rgb)   / 125.0 - 1.0);
            normal.setY(1.0 * qGreen(rgb) / 125.0 - 1.0);
            normal.setZ(1.0 * qBlue(rgb)  / 125.0 - 1.0);
            normal.normalize();
            m_normals.push_back(normal);
        }
    }

    // Load texture
    m_textureid = bindTexture(QPixmap(":/texture.png"), GL_TEXTURE_2D);

    // Create pbuffer
    m_pbuffer = new QGLPixelBuffer(QSize(img.width(), img.height()), format(), this);
    m_pbuffer_location = m_pbuffer->generateDynamicTexture();
    has_pbuffer =  m_pbuffer->bindToDynamicTexture(m_pbuffer_location);

    m_program_pbuffer.addShaderFromSourceFile(QGLShader::Fragment, ":/pbuffer.glsl");
    m_program_pbuffer.link();
    m_program_pbuffer.bind();
    m_program_pbuffer.setUniformValue("pbuffer_size", QVector2D(m_pbuffer->width(), m_pbuffer->height()));
    m_program_pbuffer.release();

    // Create FBO
    m_fbo = new QGLFramebufferObject(img.width(), img.height());

    m_program_fbo.addShaderFromSourceFile(QGLShader::Fragment, ":/fbo.glsl");
    m_program_fbo.link();
    m_program_fbo.bind();
    m_program_fbo.setUniformValue("fbo_size", QVector2D(m_pbuffer->width(), m_pbuffer->height()));
    m_program_fbo.release();

    // Vertex buffer init
    m_vertexbuffer.create();
    m_vertexbuffer.bind();
    m_vertexbuffer.allocate(m_vertices.constData(), m_vertices.size() * sizeof(QVector3D));
    m_vertexbuffer.release();

    // Normal vbo
    m_normalbuffer.create();
    m_normalbuffer.bind();
    m_normalbuffer.allocate(m_normals.constData(), sizeof(QVector3D) * m_normals.size());
    m_normalbuffer.release();

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

    // Init shader program
    m_program.addShaderFromSourceFile(QGLShader::Vertex, ":/vertex.glsl");
    m_program.addShaderFromSourceFile(QGLShader::Fragment, ":/fragment.glsl");

    m_program.bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    m_program.bindAttributeLocation("normal", PROGRAM_NORMAL_ATTRIBUTE);
    m_program.bindAttributeLocation("texture_coordonnees", PROGRAM_TEXTURE_ATTRIBUTE);

    m_program.link();

    // Light settings
    m_program.bind();
    m_program.setUniformValue("ambiant_color", QVector4D(0.0, 0.0, 0.0, 1.0));
    m_program.setUniformValue("light_direction", QVector4D(cos(0.0), 1.0, sin(0.0), 1.0));
    m_program.release();

    // GL options
    glEnable(GL_DEPTH_TEST);
    qglClearColor(Qt::darkGray);
}

void HeightmapWidget::paintGL()
{
    // Create texture
    switch(mode_rendu)
    {
    case MODE_IMAGE:
        glBindTexture(GL_TEXTURE_2D, m_textureid);
        break;

    case MODE_PBUFFER:
        m_pbuffer->makeCurrent();
        m_program_pbuffer.bind();

        m_vertexbuffer.bind();
        m_program.enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
        m_program.setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3);
        m_vertexbuffer.release();

        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_QUADS);
            glVertex2f(-1.0, -1.0);
            glVertex2f(-1.0, 1.0);
            glVertex2f(1.0, 1.0);
            glVertex2f(1.0, -1.0);
        glEnd();

        if (!has_pbuffer)
            m_pbuffer->updateDynamicTexture(m_pbuffer_location);
        m_program_pbuffer.release();

        makeCurrent();
        glBindTexture(GL_TEXTURE_2D, m_pbuffer_location);
        break;

    case MODE_FBO:
        m_program_fbo.bind();
        m_fbo->bind();

        m_vertexbuffer.bind();
        m_program.enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
        m_program.setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3);
        m_vertexbuffer.release();

        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_QUADS);
            glVertex2f(-1.0, -1.0);
            glVertex2f(-1.0, 1.0);
            glVertex2f(1.0, 1.0);
            glVertex2f(1.0, -1.0);
        glEnd();

        m_fbo->release();
        m_program_fbo.release();
        glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
        break;

    case MODE_PBUFFER_PAINTER:
        {
            QPainter pbuffer_painter(m_pbuffer);
            pbuffer_painter.fillRect(0, 0, m_pbuffer->width(), m_pbuffer->height(), QBrush(Qt::yellow));
            pbuffer_painter.setPen(QPen(Qt::black, 5));
            pbuffer_painter.drawEllipse(m_pbuffer->width()*6/16, m_pbuffer->height()*6/16, m_pbuffer->width()/16, m_pbuffer->height()/16);
            pbuffer_painter.drawEllipse(m_pbuffer->width()*9/16, m_pbuffer->height()*6/16, m_pbuffer->width()/16, m_pbuffer->height()/16);
            pbuffer_painter.drawEllipse(m_pbuffer->width()/4, m_pbuffer->height()/4, m_pbuffer->width()/2, m_pbuffer->height()/2);
            pbuffer_painter.drawArc(m_pbuffer->width()*3/8, m_pbuffer->height()*3/8, m_pbuffer->width()/4, m_pbuffer->height()/4, -30*16, -120*16);
            pbuffer_painter.end();

            makeCurrent();
            glBindTexture(GL_TEXTURE_2D, m_pbuffer_location);
        }
        break;

    case MODE_FBO_PAINTER:
        {
            QPainter fbo_painter(m_fbo);
            fbo_painter.fillRect(0, 0, m_fbo->width(), m_fbo->height(), QBrush(Qt::black));
            fbo_painter.setPen(QPen(Qt::yellow, 5));
            fbo_painter.drawEllipse(m_pbuffer->width()*6/16, m_pbuffer->height()*6/16, m_pbuffer->width()/16, m_pbuffer->height()/16);
            fbo_painter.drawEllipse(m_pbuffer->width()*9/16, m_pbuffer->height()*6/16, m_pbuffer->width()/16, m_pbuffer->height()/16);
            fbo_painter.drawEllipse(m_pbuffer->width()/4, m_pbuffer->height()/4, m_pbuffer->width()/2, m_pbuffer->height()/2);
            fbo_painter.drawArc(m_pbuffer->width()*3/8, m_pbuffer->height()*3/8, m_pbuffer->width()/4, m_pbuffer->height()/4, -30*16, -120*16);
            fbo_painter.end();

            glViewport(0, 0, width(), height());
            glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
        }
        break;
    }

    // Draw map
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_program.bind();

    // Model view proj matrices
    QMatrix4x4 projection;
    projection.perspective(30.0, 1.0 * width() / height(), 0.1, 100.0);

    QMatrix4x4 model;
    model.rotate(x_rot / 16.0, 1.0, 0.0, 0.0);
    model.rotate(y_rot / 16.0, 0.0, 1.0, 0.0);
    model.rotate(z_rot / 16.0, 0.0, 0.0, 1.0);

    QMatrix4x4 view;
    view.translate(0.0, 0.0, distance);

    m_program.setUniformValue("matrixpmv", projection * view * model);

    // Vertex VBO
    m_vertexbuffer.bind();
    m_program.enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    m_program.setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3);
    m_vertexbuffer.release();

    // Normal VBO
    m_normalbuffer.bind();
    m_program.enableAttributeArray(PROGRAM_NORMAL_ATTRIBUTE);
    m_program.setAttributeBuffer(PROGRAM_NORMAL_ATTRIBUTE, GL_FLOAT, 0, 3);
    m_normalbuffer.release();

    // Textures
    m_texturebuffer.bind();
    m_program.enableAttributeArray(PROGRAM_TEXTURE_ATTRIBUTE);
    m_program.setAttributeBuffer(PROGRAM_TEXTURE_ATTRIBUTE, GL_FLOAT, 0, 2);
    m_texturebuffer.release();

    m_indicebuffer.bind();
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
    m_indicebuffer.release();

    m_program.disableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    m_program.disableAttributeArray(PROGRAM_NORMAL_ATTRIBUTE);
    m_program.disableAttributeArray(PROGRAM_TEXTURE_ATTRIBUTE);
    m_program.release();

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
    renderText(10, 15, "FPS : " + QString::number(last_count));
    renderText(10, 30, QString(MODE_NAME[mode_rendu]));
    renderText(10, 45, QString("Map size : %1x%2").arg(m_pbuffer->size().width()).arg(m_pbuffer->size().height()));
}

void HeightmapWidget::paintEvent(QPaintEvent *)
{
    paintGL();

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.drawText(width() - 130, 20, QString("Texture :"));
    QPixmap pixmap;

    if (mode_rendu == MODE_PBUFFER_PAINTER || mode_rendu == MODE_PBUFFER)
        pixmap = QPixmap::fromImage(m_pbuffer->toImage());
    else if (mode_rendu == MODE_FBO_PAINTER || mode_rendu == MODE_FBO)
        pixmap = QPixmap::fromImage(m_fbo->toImage());
    else if (mode_rendu == MODE_IMAGE)
        pixmap = QPixmap(":/texture.png");

    painter.drawPixmap(width() - 130, 25, 100, 100, pixmap);
    painter.end();
}

void HeightmapWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void HeightmapWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space)
        mode_rendu  = static_cast<MODE_RENDU>((mode_rendu + 1) % 5);
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
