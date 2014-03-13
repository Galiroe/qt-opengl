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

#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <QVector>
#include <QTimer>
#include <QTime>
#include <QVector2D>
#include <QVector3D>
#include <QGLWidget>
#include <QGLBuffer>
#include <QGLShaderProgram>

class HeightmapWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit HeightmapWidget(QWidget *parent = 0);
    ~HeightmapWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void rotateBy(int x, int y, int z);

private:
    // Heightmap infos
    int vertices_by_x;
    int vertices_by_z;
    int quads_by_x;
    int quads_by_z;

    // FPS
    QTimer timer;
    QTime last_time;
    int frame_count;
    int last_count;

    // 3D / 2D coords
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_normals;
    QVector<QVector2D> m_textures;
    QVector<GLuint>    m_indices;
    GLuint             m_textureid;

    // GPU Buffer
    QGLBuffer m_vertexbuffer;
    QGLBuffer m_normalbuffer;
    QGLBuffer m_indicebuffer;
    QGLBuffer m_texturebuffer;

    // Shader program
    QGLShaderProgram m_program;

    // View & rotation settings
    QPoint last_pos;
    float  distance;
    float  x_rot;
    float  y_rot;
    float  z_rot;
    float  light_alpha;
    bool   light_animation;
    int    ratio_shadow;
    int    ratio_texture;
    bool   overpainting;
};

#endif // HEIGHTMAP_H
