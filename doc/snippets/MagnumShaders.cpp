/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <numeric>
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include "Magnum/GL/Buffer.h"
#include "Magnum/GL/DefaultFramebuffer.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/GL/Renderbuffer.h"
#include "Magnum/GL/RenderbufferFormat.h"
#include "Magnum/GL/Texture.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Math/Matrix3.h"
#include "Magnum/Math/Matrix4.h"
#include "Magnum/MeshTools/Duplicate.h"
#include "Magnum/Shaders/DistanceFieldVector.h"
#include "Magnum/Shaders/Flat.h"
#include "Magnum/Shaders/MeshVisualizer.h"
#include "Magnum/Shaders/Phong.h"
#include "Magnum/Shaders/Vector.h"
#include "Magnum/Shaders/VertexColor.h"

using namespace Magnum;
using namespace Magnum::Math::Literals;

int main() {

/* internal compiler error: in gimplify_init_constructor, at gimplify.c:4271
   on GCC 4.8 in the [60] array */
#if !defined(__GNUC__) || defined(__clang__) || __GNUC__*100 + __GNUC_MINOR__ >= 500
{
/* [shaders-setup] */
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 textureCoordinates;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::Phong::Position{},
    Shaders::Phong::Normal{},
    Shaders::Phong::TextureCoordinates{})
     //...
     ;
/* [shaders-setup] */

/* [shaders-rendering] */
Matrix4 transformationMatrix, projectionMatrix;
GL::Texture2D diffuseTexture, specularTexture;

Shaders::Phong shader{Shaders::Phong::Flag::DiffuseTexture};
shader.bindDiffuseTexture(diffuseTexture)
    .setLightPosition({5.0f, 5.0f, 7.0f})
    .setTransformationMatrix(transformationMatrix)
    .setNormalMatrix(transformationMatrix.normalMatrix())
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [shaders-rendering] */

/* [shaders-generic] */
mesh.addVertexBuffer(vertices, 0,
    Shaders::Generic3D::Position{},
    Shaders::Generic3D::Normal{},
    Shaders::Generic3D::TextureCoordinates{});
/* [shaders-generic] */

/* [shaders-meshvisualizer] */
Shaders::MeshVisualizer3D visualizerShader{Shaders::MeshVisualizer3D::Flag::Wireframe};
visualizerShader
    .setColor(0x2f83cc_rgbf)
    .setWireframeColor(0xdcdcdc_rgbf)
    .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
    .setTransformationMatrix(transformationMatrix)
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [shaders-meshvisualizer] */
}

{
/* [DistanceFieldVector-usage1] */
struct Vertex {
    Vector2 position;
    Vector2 textureCoordinates;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::DistanceFieldVector2D::Position{},
    Shaders::DistanceFieldVector2D::TextureCoordinates{})
    // ...
    ;
/* [DistanceFieldVector-usage1] */
}

{
GL::Mesh mesh;
/* [DistanceFieldVector-usage2] */
Matrix3 transformationMatrix, projectionMatrix;
GL::Texture2D texture;

Shaders::DistanceFieldVector2D shader;
shader.setColor(0x2f83cc_rgbf)
    .setOutlineColor(0xdcdcdc_rgbf)
    .setOutlineRange(0.6f, 0.4f)
    .bindVectorTexture(texture)
    .setTransformationProjectionMatrix(projectionMatrix*transformationMatrix)
    .draw(mesh);
/* [DistanceFieldVector-usage2] */
}

{
/* [Flat-usage-colored1] */
struct Vertex {
    Vector3 position;
};
Vertex data[60]{
    //...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0, Shaders::Flat3D::Position{})
    // ...
    ;
/* [Flat-usage-colored1] */

/* [Flat-usage-colored2] */
Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-5.0f));
Matrix4 projectionMatrix =
    Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);

Shaders::Flat3D shader;
shader.setColor(0x2f83cc_rgbf)
    .setTransformationProjectionMatrix(projectionMatrix*transformationMatrix)
    .draw(mesh);
/* [Flat-usage-colored2] */
}

{
/* [Flat-usage-textured1] */
struct Vertex {
    Vector3 position;
    Vector2 textureCoordinates;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::Flat3D::Position{},
    Shaders::Flat3D::TextureCoordinates{})
    // ...
    ;
/* [Flat-usage-textured1] */

/* [Flat-usage-textured2] */
Matrix4 transformationMatrix, projectionMatrix;
GL::Texture2D texture;

Shaders::Flat3D shader{Shaders::Flat3D::Flag::Textured};
shader.setTransformationProjectionMatrix(projectionMatrix*transformationMatrix)
    .bindTexture(texture)
    .draw(mesh);
/* [Flat-usage-textured2] */
}

#ifndef MAGNUM_TARGET_GLES2
{
GL::Framebuffer framebuffer{{}};
GL::Mesh mesh;
Vector2i size;
UnsignedInt meshId{};
/* [Flat-usage-object-id] */
GL::Renderbuffer color, objectId;
color.setStorage(GL::RenderbufferFormat::RGBA8, size);
objectId.setStorage(GL::RenderbufferFormat::R16UI, size); // large as needed
framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, color)
    .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, objectId);

Shaders::Flat3D shader{Shaders::Flat3D::Flag::ObjectId};

// ...

framebuffer.mapForDraw({
        {Shaders::Flat3D::ColorOutput, GL::Framebuffer::ColorAttachment{0}},
        {Shaders::Flat3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}})
    .clearColor(0, 0x1f1f1f_rgbf)
    .clearColor(1, Vector4ui{0})
    .bind();

shader.setObjectId(meshId)
    .draw(mesh);
/* [Flat-usage-object-id] */

/* [shaders-generic-object-id] */
framebuffer.mapForDraw({
    {Shaders::Generic3D::ColorOutput, GL::Framebuffer::ColorAttachment{0}},
    {Shaders::Generic3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});
/* [shaders-generic-object-id] */
}
#endif

{
/* [MeshVisualizer-usage-geom1] */
struct Vertex {
    Vector3 position;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0, Shaders::MeshVisualizer3D::Position{});
/* [MeshVisualizer-usage-geom1] */

/* [MeshVisualizer-usage-geom2] */
Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-5.0f));
Matrix4 projectionMatrix =
    Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);

Shaders::MeshVisualizer3D shader{Shaders::MeshVisualizer3D::Flag::Wireframe};
shader.setColor(0x2f83cc_rgbf)
    .setWireframeColor(0xdcdcdc_rgbf)
    .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
    .setTransformationMatrix(transformationMatrix)
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [MeshVisualizer-usage-geom2] */

/* [MeshVisualizer-usage-no-geom-old] */
Containers::Array<Float> vertexIndex{Containers::arraySize(data)};
std::iota(vertexIndex.begin(), vertexIndex.end(), 0.0f);

GL::Buffer vertexIndices;
vertexIndices.setData(vertexIndex, GL::BufferUsage::StaticDraw);

mesh.addVertexBuffer(vertexIndices, 0, Shaders::MeshVisualizer3D::VertexIndex{});
/* [MeshVisualizer-usage-no-geom-old] */
}
#endif

#if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
/* internal compiler error: in gimplify_init_constructor, at gimplify.c:4271
   on GCC 4.8 in the [60] array */
#if !defined(__GNUC__) || defined(__clang__) || __GNUC__*100 + __GNUC_MINOR__ >= 500
{
/* [MeshVisualizer-usage-tbn1] */
struct Vertex {
    Vector3 position;
    Vector4 tangent;
    Vector3 normal;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::MeshVisualizer3D::Position{},
    Shaders::MeshVisualizer3D::Tangent4{},
    Shaders::MeshVisualizer3D::Normal{});
/* [MeshVisualizer-usage-tbn1] */

/* [MeshVisualizer-usage-tbn2] */
Matrix4 transformationMatrix, projectionMatrix;

Shaders::MeshVisualizer3D shader{
    Shaders::MeshVisualizer3D::Flag::TangentDirection|
    Shaders::MeshVisualizer3D::Flag::BitangentFromTangentDirection|
    Shaders::MeshVisualizer3D::Flag::NormalDirection};
shader.setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
    .setTransformationMatrix(transformationMatrix)
    .setProjectionMatrix(projectionMatrix)
    .setNormalMatrix(transformationMatrix.normalMatrix())
    .setLineLength(0.3f)
    .draw(mesh);
/* [MeshVisualizer-usage-tbn2] */

}
#endif
#endif

{
/* [MeshVisualizer-usage-no-geom1] */
Containers::StridedArrayView1D<const UnsignedInt> indices;
Containers::StridedArrayView1D<const Vector3> indexedPositions;

/* De-indexing the position array */
GL::Buffer vertices{MeshTools::duplicate(indices, indexedPositions)};

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0, Shaders::MeshVisualizer3D::Position{});
/* [MeshVisualizer-usage-no-geom1] */
}

{
GL::Mesh mesh;
/* [MeshVisualizer-usage-no-geom2] */
Matrix4 transformationMatrix, projectionMatrix;

Shaders::MeshVisualizer3D shader{
    Shaders::MeshVisualizer3D::Flag::Wireframe|
    Shaders::MeshVisualizer3D::Flag::NoGeometryShader};
shader.setColor(0x2f83cc_rgbf)
    .setWireframeColor(0xdcdcdc_rgbf)
    .setTransformationMatrix(transformationMatrix)
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [MeshVisualizer-usage-no-geom2] */
}
#if !defined(__GNUC__) || defined(__clang__) || __GNUC__*100 + __GNUC_MINOR__ >= 500
{
/* [Phong-usage-colored1] */
struct Vertex {
    Vector3 position;
    Vector3 normal;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::Phong::Position{},
    Shaders::Phong::Normal{});
/* [Phong-usage-colored1] */

/* [Phong-usage-colored2] */
Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-5.0f));
Matrix4 projectionMatrix =
    Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);

Shaders::Phong shader;
shader.setDiffuseColor(0x2f83cc_rgbf)
    .setShininess(200.0f)
    .setLightPosition({5.0f, 5.0f, 7.0f})
    .setTransformationMatrix(transformationMatrix)
    .setNormalMatrix(transformationMatrix.normalMatrix())
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [Phong-usage-colored2] */
}

{
/* [Phong-usage-texture1] */
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 textureCoordinates;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::Phong::Position{},
    Shaders::Phong::Normal{},
    Shaders::Phong::TextureCoordinates{});
/* [Phong-usage-texture1] */

/* [Phong-usage-texture2] */
Matrix4 transformationMatrix, projectionMatrix;
GL::Texture2D diffuseTexture, specularTexture;

Shaders::Phong shader{Shaders::Phong::Flag::DiffuseTexture|
                      Shaders::Phong::Flag::SpecularTexture};
shader.bindTextures(nullptr, &diffuseTexture, &specularTexture, nullptr)
    .setLightPosition({5.0f, 5.0f, 7.0f})
    .setTransformationMatrix(transformationMatrix)
    .setNormalMatrix(transformationMatrix.normalMatrix())
    .setProjectionMatrix(projectionMatrix)
    .draw(mesh);
/* [Phong-usage-texture2] */
}
#endif

{
GL::Texture2D diffuseAlphaTexture;
Color3 diffuseRgb, specularRgb;
/* [Phong-usage-alpha] */
Shaders::Phong shader{Shaders::Phong::Flag::AmbientTexture|
                      Shaders::Phong::Flag::DiffuseTexture};
shader.bindTextures(&diffuseAlphaTexture, &diffuseAlphaTexture, nullptr, nullptr)
    .setAmbientColor(0x000000ff_rgbaf)
    .setDiffuseColor(Color4{diffuseRgb, 0.0f})
    .setSpecularColor(Color4{specularRgb, 0.0f});
/* [Phong-usage-alpha] */
}

#if !defined(__GNUC__) || defined(__clang__) || __GNUC__*100 + __GNUC_MINOR__ >= 500
{
/* [Vector-usage1] */
struct Vertex {
    Vector2 position;
    Vector2 textureCoordinates;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::Vector2D::Position{},
    Shaders::Vector2D::TextureCoordinates{});
/* [Vector-usage1] */

/* [Vector-usage2] */
Matrix3 transformationMatrix, projectionMatrix;
GL::Texture2D texture;

Shaders::Vector2D shader;
shader.setColor(0x2f83cc_rgbf)
    .bindVectorTexture(texture)
    .setTransformationProjectionMatrix(projectionMatrix*transformationMatrix)
    .draw(mesh);
/* [Vector-usage2] */
}

{
/* [VertexColor-usage1] */
struct Vertex {
    Vector3 position;
    Color3 color;
};
Vertex data[60]{
    // ...
};

GL::Buffer vertices;
vertices.setData(data, GL::BufferUsage::StaticDraw);

GL::Mesh mesh;
mesh.addVertexBuffer(vertices, 0,
    Shaders::VertexColor3D::Position{},
    Shaders::VertexColor3D::Color3{});
/* [VertexColor-usage1] */

/* [VertexColor-usage2] */
Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-5.0f));
Matrix4 projectionMatrix =
    Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);

Shaders::VertexColor3D shader;
shader.setTransformationProjectionMatrix(projectionMatrix*transformationMatrix)
    .draw(mesh);
/* [VertexColor-usage2] */
}
#endif

}
