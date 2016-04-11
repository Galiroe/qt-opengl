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
#include <cmath>
#include <QMatrix4x4>
#include <QPainter>

const double PI = 3.141592653589793238463;

const float MAP_SIZE = 5.0f;
enum class Attribute { Vertex, Normal, Texture };

HeightmapWidget::HeightmapWidget(QWindow* parent) :
    QOpenGLWindow(NoPartialUpdate, parent)
{
    resize(1000, 800);

    // Time
    time.start();
}

HeightmapWidget::~HeightmapWidget()
{
    //    glDeleteTextures(m_textureid);
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
    // init OpenGL
    initializeOpenGLFunctions();

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
    for(size_t z = 0; z < vertices_by_z; ++z)
    {
        for(size_t x = 0; x < vertices_by_x; ++x)
        {
            const QRgb color = img.pixel(x, z);

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
    for (size_t z = 0; z < quads_by_z; ++z)
    {
        for (size_t x = 0; x < quads_by_x; ++x)
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
    for(size_t z = 0; z < vertices_by_z; ++z)
    {
        for(size_t x = 0; x < vertices_by_x; ++x)
        {
            QVector3D normal;
            const QRgb rgb = img.pixel(x, z);
            normal.setX(1.0 * qRed(rgb)   / 125.0 - 1.0);
            normal.setY(1.0 * qGreen(rgb) / 125.0 - 1.0);
            normal.setZ(1.0 * qBlue(rgb)  / 125.0 - 1.0);
            normal.normalize();
            m_normals.push_back(normal);
        }
    }

    // Load texture
    //    m_textureid = bindTexture(QPixmap(":/texture.png"), GL_TEXTURE_2D);

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
    m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex.glsl");
    m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment.glsl");

    m_program.bindAttributeLocation("vertex", static_cast<int>(Attribute::Vertex));
    m_program.bindAttributeLocation("normal", static_cast<int>(Attribute::Normal));
    m_program.bindAttributeLocation("texture_coordonnees", static_cast<int>(Attribute::Texture));

    m_program.link();

    // Light settings
    m_program.bind();
    m_program.setUniformValue("ambiant_color", QVector4D(0.0, 0.0, 0.0, 1.0));
    m_program.setUniformValue("light_direction", QVector4D(cos(0.0), 1.0, sin(0.0), 1.0));
    m_program.release();

    // GL options
    glClearColor(0.52f, 0.52f, 0.52f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void HeightmapWidget::paintGL()
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program.bind();

    // Model view proj matrices
    QMatrix4x4 projection;
    projection.perspective(30.0f, 1.0f * width() / height(), 0.1f, 100.0f);

    QMatrix4x4 model;
    model.rotate(x_rot / 16.0f, QVector3D(1.0f, 0.0f, 0.0f));
    model.rotate(y_rot / 16.0f, QVector3D(0.0f, 1.0f, 0.0f));
    model.rotate(z_rot / 16.0f, QVector3D(0.0f, 0.0f, 1.0f));

    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, distance);

    m_program.setUniformValue("matrixpmv", projection * view * model);

    // Vertex VBO
    m_vertexbuffer.bind();
    m_program.enableAttributeArray(static_cast<int>(Attribute::Vertex));
    m_program.setAttributeBuffer(static_cast<int>(Attribute::Vertex), GL_FLOAT, 0, 3);
    m_vertexbuffer.release();

    // Normal VBO
    m_normalbuffer.bind();
    m_program.enableAttributeArray(static_cast<int>(Attribute::Normal));
    m_program.setAttributeBuffer(static_cast<int>(Attribute::Normal), GL_FLOAT, 0, 3);
    m_normalbuffer.release();

    // Texture VBO
    m_texturebuffer.bind();
    m_program.enableAttributeArray(static_cast<int>(Attribute::Texture));
    m_program.setAttributeBuffer(static_cast<int>(Attribute::Texture), GL_FLOAT, 0, 2);
    m_texturebuffer.release();

    m_indicebuffer.bind();
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
    m_indicebuffer.release();

    m_program.disableAttributeArray(static_cast<int>(Attribute::Vertex));
    m_program.disableAttributeArray(static_cast<int>(Attribute::Normal));
    m_program.disableAttributeArray(static_cast<int>(Attribute::Texture));

    // update light
    if (light_animation)
    {
        light_alpha += 0.02;
        if (light_alpha > 2*PI) light_alpha -= 2*PI;
        m_program.setUniformValue("light_direction", QVector4D(cos(light_alpha), 1.0, sin(light_alpha), 1.0));
    }

    m_program.release();

    // FPS count
    ++frame_count;
    const auto elapsed = time.elapsed();
    if (elapsed >= 1000)
    {
        last_count = frame_count;
        frame_count = 0;
        time.restart();
    }

    // Debug display
    if (overpainting)
    {
        glPolygonMode(GL_FRONT, GL_FILL);
        QPainter painter(this);
        painter.setPen(Qt::white);

        painter.drawText(10, 20, "Press Up/Down to set shadow ratio");
        painter.drawText(10, 40, "Press Left/Right to set texture ratio");
        painter.drawText(10, 60, "Press spacebar to show on/off overpainting");
        painter.drawText(10, 80, "Press L to anime on/off light");

        painter.drawText(10, height()-100, QString("FPS:%1").arg(last_count));
        painter.drawText(10, height()-80, QString("Shadow ration : %1 %").arg(ratio_shadow));
        painter.drawText(10, height()-60, QString("Texture ration : %1 %").arg(ratio_texture));
        painter.drawText(10, height()-40, "Size points : " + QString::number(m_indices.size()));
        painter.drawText(10, height()-20, QString("Rotation x:%1 y:%2 z:%3").arg(x_rot/16).arg(y_rot/16).arg(z_rot/16));

        painter.drawText(width() - 260, 20, QString("Heightmap :"));
        painter.drawPixmap(width() - 260, 25, 100, 100, QPixmap(":/heightmap.png"));

        painter.drawText(width() - 130, 20, QString("Texture :"));
        painter.drawPixmap(width() - 130, 25, 100, 100, QPixmap(":/texture.png"));
    }

    update();
}

void HeightmapWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void HeightmapWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up)
    {
        if (ratio_shadow < 100) ratio_shadow += 5;
        m_program.bind();
        m_program.setUniformValue("ratio_shadow", static_cast<GLfloat>(ratio_shadow / 100.0));
        m_program.release();
    }

    if (event->key() == Qt::Key_Down)
    {
        if (ratio_shadow > 0) ratio_shadow -= 5;
        m_program.bind();
        m_program.setUniformValue("ratio_shadow", static_cast<GLfloat>(ratio_shadow / 100.0));
        m_program.release();
    }

    if (event->key() == Qt::Key_Right)
    {
        if (ratio_texture < 100) ratio_texture += 5;
        m_program.bind();
        m_program.setUniformValue("ratio_texture", static_cast<GLfloat>(ratio_texture / 100.0));
        m_program.release();
    }

    if (event->key() == Qt::Key_Left)
    {
        if (ratio_texture > 0) ratio_texture -= 5;
        m_program.bind();
        m_program.setUniformValue("ratio_texture", static_cast<GLfloat>(ratio_texture / 100.0));
        m_program.release();
    }

    if (event->key() == Qt::Key_L)
        light_animation = !light_animation;

    if (event->key() == Qt::Key_Space)
        overpainting = !overpainting;
}

void HeightmapWidget::mousePressEvent(QMouseEvent *event)
{
    last_pos = event->pos();
}

void HeightmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    const auto dx = event->x() - last_pos.x();
    const auto dy = event->y() - last_pos.y();

    if (event->buttons() & Qt::RightButton)
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
