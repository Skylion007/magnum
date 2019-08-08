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

#include <sstream>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Directory.h>

#include "Magnum/Image.h"
#include "Magnum/ImageView.h"
#include "Magnum/PixelFormat.h"
#include "Magnum/DebugTools/CompareImage.h"
#include "Magnum/GL/Context.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/GL/OpenGLTester.h"
#include "Magnum/GL/Renderer.h"
#include "Magnum/GL/Renderbuffer.h"
#include "Magnum/GL/RenderbufferFormat.h"
#include "Magnum/GL/Texture.h"
#include "Magnum/GL/TextureFormat.h"
#include "Magnum/MeshTools/Compile.h"
#include "Magnum/MeshTools/Transform.h"
#include "Magnum/Primitives/Plane.h"
#include "Magnum/Primitives/UVSphere.h"
#include "Magnum/Shaders/Phong.h"
#include "Magnum/Trade/AbstractImporter.h"
#include "Magnum/Trade/MeshData3D.h"
#include "Magnum/Trade/ImageData.h"

#include "configure.h"

namespace Magnum { namespace Shaders { namespace Test { namespace {

struct PhongGLTest: GL::OpenGLTester {
    explicit PhongGLTest();

    void construct();

    void constructMove();

    void bindTexturesNotEnabled();
    void setAlphaMaskNotEnabled();
    void setWrongLightCount();
    void setWrongLightId();

    void renderSetup();
    void renderTeardown();

    void renderDefaults();
    void renderColored();
    void renderSinglePixelTextured();

    void renderTextured();
    void renderTexturedNormal();

    void renderShininess();

    void renderAlphaSetup();
    void renderAlphaTeardown();

    void renderAlpha();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _manager{"nonexistent"};

        GL::Renderbuffer _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};
};

/*
    Rendering tests done on:

    -   Mesa Intel
    -   Mesa AMD
    -   SwiftShader ES2/ES3
    -   ARM Mali (Huawei P10) ES2/ES3
    -   WebGL 1 / 2 (on Mesa Intel)

    Mesa AMD, SwiftShader and ARM Mali has a bigger ring with shininess = 0.
    Mesa Intel not. Currently handled as XFAIL on those, but probably could
    invert that and XFAIL on Intel?
*/

constexpr struct {
    const char* name;
    Phong::Flags flags;
    UnsignedInt lightCount;
} ConstructData[]{
    {"", {}, 1},
    {"ambient texture", Phong::Flag::AmbientTexture, 1},
    {"diffuse texture", Phong::Flag::DiffuseTexture, 1},
    {"specular texture", Phong::Flag::SpecularTexture, 1},
    {"normal texture", Phong::Flag::NormalTexture, 1},
    {"ambient + diffuse texture", Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture, 1},
    {"ambient + specular texture", Phong::Flag::AmbientTexture|Phong::Flag::SpecularTexture, 1},
    {"diffuse + specular texture", Phong::Flag::DiffuseTexture|Phong::Flag::SpecularTexture, 1},
    {"ambient + diffuse + specular texture", Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::SpecularTexture, 1},
    {"ambient + diffuse + specular + normal texture", Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::SpecularTexture|Phong::Flag::NormalTexture, 1},
    {"alpha mask", Phong::Flag::AlphaMask, 1},
    {"alpha mask + diffuse texture", Phong::Flag::AlphaMask|Phong::Flag::DiffuseTexture, 1},
    {"five lights", {}, 5}
};

using namespace Math::Literals;

const struct {
    const char* name;
    Deg rotation;
    Color4 lightColor1, lightColor2;
    Float lightPosition1, lightPosition2;
} RenderColoredData[]{
    {"", {}, 0x993366_rgbf, 0x669933_rgbf, -3.0f, 3.0f},
    {"flip lights", {}, 0x669933_rgbf, 0x993366_rgbf, 3.0f, -3.0f},
    {"rotated", 45.0_degf, 0x993366_rgbf, 0x669933_rgbf, -3.0f, 3.0f}
};

constexpr struct {
    const char* name;
    bool multiBind;
} RenderSinglePixelTexturedData[]{
    {"", false},
    {"multi bind", true}
};

constexpr struct {
    const char* name;
    const char* expected;
    Phong::Flags flags;
} RenderTexturedData[]{
    {"all", "textured.tga", Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::SpecularTexture},
    {"ambient", "textured-ambient.tga", Phong::Flag::AmbientTexture},
    {"diffuse", "textured-diffuse.tga", Phong::Flag::DiffuseTexture},
    {"specular", "textured-specular.tga", Phong::Flag::SpecularTexture}
};

/* MSVC 2015 doesn't like constexpr here due to the angles */
const struct {
    const char* name;
    bool multiBind;
    Deg rotation;
} RenderTexturedNormalData[]{
    {"", false, {}},
    {"multi bind", true, {}},
    {"rotated 90°", false, 90.0_degf},
    {"rotated -90°", false, -90.0_degf}
};

const struct {
    const char* name;
    const char* expected;
    Float shininess;
    Color4 specular;
} RenderShininessData[] {
    {"80", "shininess80.tga", 80.0f, 0xffffff_rgbf},
    {"10", "shininess10.tga", 10.0f, 0xffffff_rgbf},
    {"0", "shininess0.tga", 0.0f, 0xffffff_rgbf},
    {"0.001", "shininess0.tga", 0.001f, 0xffffff_rgbf},
    {"black specular", "shininess-black-specular.tga", 80.0f, 0x000000_rgbf}
};

const struct {
    const char* name;
    const char* expected;
    bool blending;
    Phong::Flags flags;
    Float threshold;
    const char* ambientTexture;
    const char* diffuseTexture;
    Color4 ambientColor;
    Color4 diffuseColor;
} RenderAlphaData[] {
    /* All those deliberately have a non-white diffuse in order to match the
       expected data from textured() */
    {"none, separate", "PhongTestFiles/textured-diffuse.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture, 0.0f,
        "alpha-texture.tga", "diffuse-texture.tga",
        0xffffffff_rgbaf, 0x9999ff00_rgbaf},
    {"none, combined", "PhongTestFiles/textured-diffuse.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture, 0.0f,
        "diffuse-alpha-texture.tga", "diffuse-alpha-texture.tga",
        0x000000ff_rgbaf, 0x9999ff00_rgbaf},
    {"blending, separate", "PhongTestFiles/textured-diffuse-alpha.tga", true,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture, 0.0f,
        "alpha-texture.tga", "diffuse-texture.tga",
        0xffffffff_rgbaf, 0x9999ff00_rgbaf},
    {"blending, combined", "PhongTestFiles/textured-diffuse-alpha.tga", true,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture, 0.0f,
        "diffuse-alpha-texture.tga", "diffuse-alpha-texture.tga",
        0x000000ff_rgbaf, 0x9999ff00_rgbaf},
    {"masking 0.0, separate", "PhongTestFiles/textured-diffuse.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::AlphaMask, 0.0f,
        "alpha-texture.tga", "diffuse-texture.tga",
        0xffffffff_rgbaf, 0x9999ff00_rgbaf},
    {"masking 0.5, separate", "PhongTestFiles/textured-diffuse-alpha-mask0.5.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::AlphaMask, 0.5f,
        "alpha-texture.tga", "diffuse-texture.tga",
        0xffffffff_rgbaf, 0x9999ff00_rgbaf},
    {"masking 0.5, combined", "PhongTestFiles/textured-diffuse-alpha-mask0.5.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::AlphaMask, 0.5f,
        "diffuse-alpha-texture.tga", "diffuse-alpha-texture.tga",
        0x000000ff_rgbaf, 0x9999ff00_rgbaf},
    {"masking 1.0, separate", "TestFiles/alpha-mask1.0.tga", false,
        Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::AlphaMask, 1.0f,
        "alpha-texture.tga", "diffuse-texture.tga",
        0xffffffff_rgbaf, 0x9999ff00_rgbaf}
};

PhongGLTest::PhongGLTest() {
    addInstancedTests({&PhongGLTest::construct}, Containers::arraySize(ConstructData));

    addTests({&PhongGLTest::constructMove,

              &PhongGLTest::bindTexturesNotEnabled,
              &PhongGLTest::setAlphaMaskNotEnabled,
              &PhongGLTest::setWrongLightCount,
              &PhongGLTest::setWrongLightId});

    addTests({&PhongGLTest::renderDefaults},
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderColored},
        Containers::arraySize(RenderColoredData),
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderSinglePixelTextured},
        Containers::arraySize(RenderSinglePixelTexturedData),
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderTextured},
        Containers::arraySize(RenderTexturedData),
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderTexturedNormal},
        Containers::arraySize(RenderTexturedNormalData),
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderShininess},
        Containers::arraySize(RenderShininessData),
        &PhongGLTest::renderSetup,
        &PhongGLTest::renderTeardown);

    addInstancedTests({&PhongGLTest::renderAlpha},
        Containers::arraySize(RenderAlphaData),
        &PhongGLTest::renderAlphaSetup,
        &PhongGLTest::renderAlphaTeardown);

    /* Load the plugins directly from the build tree. Otherwise they're either
       static and already loaded or not present in the build tree */
    #ifdef ANYIMAGEIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT(_manager.load(ANYIMAGEIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
    #ifdef TGAIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT(_manager.load(TGAIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
}

void PhongGLTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Phong shader{data.flags, data.lightCount};
    CORRADE_COMPARE(shader.flags(), data.flags);
    CORRADE_COMPARE(shader.lightCount(), data.lightCount);
    CORRADE_VERIFY(shader.id());
    {
        #ifdef CORRADE_TARGET_APPLE
        CORRADE_EXPECT_FAIL("macOS drivers need insane amount of state to validate properly.");
        #endif
        CORRADE_VERIFY(shader.validate().first);
    }
}

void PhongGLTest::constructMove() {
    Phong a{Phong::Flag::AlphaMask, 3};
    const GLuint id = a.id();
    CORRADE_VERIFY(id);

    MAGNUM_VERIFY_NO_GL_ERROR();

    Phong b{std::move(a)};
    CORRADE_COMPARE(b.id(), id);
    CORRADE_COMPARE(b.flags(), Phong::Flag::AlphaMask);
    CORRADE_COMPARE(b.lightCount(), 3);
    CORRADE_VERIFY(!a.id());

    Phong c{NoCreate};
    c = std::move(b);
    CORRADE_COMPARE(c.id(), id);
    CORRADE_COMPARE(c.flags(), Phong::Flag::AlphaMask);
    CORRADE_COMPARE(c.lightCount(), 3);
    CORRADE_VERIFY(!b.id());
}

void PhongGLTest::bindTexturesNotEnabled() {
    std::ostringstream out;
    Error redirectError{&out};

    GL::Texture2D texture;
    Phong shader;
    shader.bindAmbientTexture(texture)
          .bindDiffuseTexture(texture)
          .bindSpecularTexture(texture)
          .bindNormalTexture(texture)
          .bindTextures(&texture, &texture, &texture, &texture);

    CORRADE_COMPARE(out.str(),
        "Shaders::Phong::bindAmbientTexture(): the shader was not created with ambient texture enabled\n"
        "Shaders::Phong::bindDiffuseTexture(): the shader was not created with diffuse texture enabled\n"
        "Shaders::Phong::bindSpecularTexture(): the shader was not created with specular texture enabled\n"
        "Shaders::Phong::bindNormalTexture(): the shader was not created with normal texture enabled\n"
        "Shaders::Phong::bindTextures(): the shader was not created with any textures enabled\n");
}

void PhongGLTest::setAlphaMaskNotEnabled() {
    std::ostringstream out;
    Error redirectError{&out};

    Phong shader;
    shader.setAlphaMask(0.75f);

    CORRADE_COMPARE(out.str(),
        "Shaders::Phong::setAlphaMask(): the shader was not created with alpha mask enabled\n");
}

void PhongGLTest::setWrongLightCount() {
    std::ostringstream out;
    Error redirectError{&out};

    Phong shader{{}, 5};

    /* This is okay */
    shader.setLightColors({{}, {}, {}, {}, {}})
        .setLightPositions({{}, {}, {}, {}, {}});

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* This is not */
    shader.setLightColor({})
        .setLightPosition({});

    CORRADE_COMPARE(out.str(),
        "Shaders::Phong::setLightColors(): expected 5 items but got 1\n"
        "Shaders::Phong::setLightPositions(): expected 5 items but got 1\n");
}

void PhongGLTest::setWrongLightId() {
    std::ostringstream out;
    Error redirectError{&out};

    Phong shader{{}, 3};

    /* This is okay */
    shader.setLightColor(2, {})
        .setLightPosition(2, {});

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* This is not */
    shader.setLightColor(3, {})
        .setLightPosition(3, {});

    CORRADE_COMPARE(out.str(),
        "Shaders::Phong::setLightColor(): light ID 3 is out of bounds for 3 lights\n"
        "Shaders::Phong::setLightPosition(): light ID 3 is out of bounds for 3 lights\n");
}

constexpr Vector2i RenderSize{80, 80};

void PhongGLTest::renderSetup() {
    /* Pick a color that's directly representable on RGBA4 as well to reduce
       artifacts */
    GL::Renderer::setClearColor(0x111111_rgbf);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _color = GL::Renderbuffer{};
    _color.setStorage(
        #if !defined(MAGNUM_TARGET_GLES2) || !defined(MAGNUM_TARGET_WEBGL)
        GL::RenderbufferFormat::RGBA8,
        #else
        GL::RenderbufferFormat::RGBA4,
        #endif
        RenderSize);
    _framebuffer = GL::Framebuffer{{{}, RenderSize}};
    _framebuffer
        .attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _color)
        .clear(GL::FramebufferClear::Color)
        .bind();
}

void PhongGLTest::renderTeardown() {
    _color = GL::Renderbuffer{NoCreate};
    _framebuffer = GL::Framebuffer{NoCreate};
}

void PhongGLTest::renderDefaults() {
    /* The light is at the center by default, so we scale the sphere to half
       and  move the vertices back a bit to avoid a fully-black render but
       still have the thing in the default [-1; 1] cube */
    Trade::MeshData3D meshData = Primitives::uvSphereSolid(16, 32);
    Matrix4 transformation =
        Matrix4::translation(Vector3::zAxis(-1.0f))*Matrix4::scaling(Vector3(1.0f, 1.0f, 0.25f));
    MeshTools::transformPointsInPlace(transformation, meshData.positions(0));
    /** @todo use Matrix4::normalMatrix() */
    MeshTools::transformVectorsInPlace(transformation.inverted().transposed(), meshData.normals(0));
    GL::Mesh sphere = MeshTools::compile(meshData);

    Phong shader;
    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* SwiftShader has 6 different pixels on the edges and a bunch of small
       rounding errors */
    const Float maxThreshold = 31.0f, meanThreshold = 0.122f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
    const Float maxThreshold = 31.0f, meanThreshold = 4.142f;
    #endif
    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Directory::join(SHADERS_TEST_DIR, "PhongTestFiles/defaults.tga"),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

void PhongGLTest::renderColored() {
    auto&& data = RenderColoredData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Mesh sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32));

    Phong shader{{}, 2};
    shader.setLightColors({data.lightColor1, data.lightColor2})
        .setLightPositions({{data.lightPosition1, -3.0f, 0.0f},
                            {data.lightPosition2, -3.0f, 0.0f}})
        .setAmbientColor(0x330033_rgbf)
        .setDiffuseColor(0xccffcc_rgbf)
        .setSpecularColor(0x6666ff_rgbf)
        .setTransformationMatrix(Matrix4::translation(Vector3::zAxis(-2.15f))*
                                 Matrix4::rotationY(data.rotation))
        .setNormalMatrix(Matrix4::rotationY(data.rotation).rotationScaling())
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f));

    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* SwiftShader has some minor rounding differences (max = 1). ARM Mali G71
       has bigger rounding differences. */
    const Float maxThreshold = 8.34f, meanThreshold = 0.066f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
    const Float maxThreshold = 15.34f, meanThreshold = 3.33f;
    #endif
    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Directory::join(SHADERS_TEST_DIR, "PhongTestFiles/colored.tga"),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

constexpr GL::TextureFormat TextureFormatRGB =
    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    GL::TextureFormat::RGB8
    #else
    GL::TextureFormat::RGB
    #endif
    ;
constexpr GL::TextureFormat TextureFormatRGBA =
    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    GL::TextureFormat::RGBA8
    #else
    GL::TextureFormat::RGBA
    #endif
    ;

void PhongGLTest::renderSinglePixelTextured() {
    auto&& data = RenderSinglePixelTexturedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Mesh sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32,
        Primitives::UVSphereTextureCoords::Generate));

    const Color4ub ambientData[]{ 0x330033_rgb };
    ImageView2D ambientImage{PixelFormat::RGBA8Unorm, Vector2i{1}, ambientData};
    GL::Texture2D ambient;
    ambient.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, TextureFormatRGBA, Vector2i{1})
        .setSubImage(0, {}, ambientImage);

    const Color4ub diffuseData[]{ 0xccffcc_rgb };
    ImageView2D diffuseImage{PixelFormat::RGBA8Unorm, Vector2i{1}, diffuseData};
    GL::Texture2D diffuse;
    diffuse.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, TextureFormatRGBA, Vector2i{1})
        .setSubImage(0, {}, diffuseImage);

    const Color4ub specularData[]{ 0x6666ff_rgb };
    ImageView2D specularImage{PixelFormat::RGBA8Unorm, Vector2i{1}, specularData};
    GL::Texture2D specular;
    specular.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, TextureFormatRGBA, Vector2i{1})
        .setSubImage(0, {}, specularImage);

    Phong shader{Phong::Flag::AmbientTexture|Phong::Flag::DiffuseTexture|Phong::Flag::SpecularTexture, 2};
    shader.setLightColors({0x993366_rgbf, 0x669933_rgbf})
        .setLightPositions({{-3.0f, -3.0f, 0.0f},
                            { 3.0f, -3.0f, 0.0f}})
        .setTransformationMatrix(Matrix4::translation(Vector3::zAxis(-2.15f)))
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f));

    if(data.multiBind)
        shader.bindTextures(&ambient, &diffuse, &specular, nullptr);
    else shader
        .bindAmbientTexture(ambient)
        .bindDiffuseTexture(diffuse)
        .bindSpecularTexture(specular);

    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* SwiftShader has some minor rounding differences (max = 1). ARM Mali G71
       has bigger rounding differences. */
    const Float maxThreshold = 7.0f, meanThreshold = 0.066f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
    const Float maxThreshold = 15.34f, meanThreshold = 3.33f;
    #endif
    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Directory::join(SHADERS_TEST_DIR, "PhongTestFiles/colored.tga"),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

void PhongGLTest::renderTextured() {
    auto&& data = RenderTexturedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    GL::Mesh sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32,
        Primitives::UVSphereTextureCoords::Generate));

    Phong shader{data.flags, 2};

    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer);

    GL::Texture2D ambient;
    if(data.flags & Phong::Flag::AmbientTexture) {
        Containers::Optional<Trade::ImageData2D> image;
        CORRADE_VERIFY(importer->openFile(Utility::Directory::join(SHADERS_TEST_DIR, "TestFiles/ambient-texture.tga")) && (image = importer->image2D(0)));
        ambient.setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, TextureFormatRGB, image->size())
            .setSubImage(0, {}, *image);
        shader
            .bindAmbientTexture(ambient)
            /* Colorized. Case without a color (where it should be white) is
               tested in renderSinglePixelTextured() */
            .setAmbientColor(0xff9999_rgbf);
    }

    /* If no diffuse texture is present, dial down the default diffuse color
       so ambient/specular is visible */
    GL::Texture2D diffuse;
    if(data.flags & Phong::Flag::DiffuseTexture) {
        Containers::Optional<Trade::ImageData2D> image;
        CORRADE_VERIFY(importer->openFile(Utility::Directory::join(SHADERS_TEST_DIR, "TestFiles/diffuse-texture.tga")) && (image = importer->image2D(0)));
        diffuse.setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, TextureFormatRGB, image->size())
            .setSubImage(0, {}, *image);
        shader
            .bindDiffuseTexture(diffuse)
            /* Colorized. Case without a color (where it should be white) is
               tested in renderSinglePixelTextured() */
            .setDiffuseColor(0x9999ff_rgbf);
    } else shader.setDiffuseColor(0x333333_rgbf);

    GL::Texture2D specular;
    if(data.flags & Phong::Flag::SpecularTexture) {
        Containers::Optional<Trade::ImageData2D> image;
        CORRADE_VERIFY(importer->openFile(Utility::Directory::join(SHADERS_TEST_DIR, "TestFiles/specular-texture.tga")) && (image = importer->image2D(0)));
        specular.setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, TextureFormatRGB, image->size())
            .setSubImage(0, {}, *image);
        shader
            .bindSpecularTexture(specular)
            /* Colorized. Case without a color (where it should be white) is
               tested in renderSinglePixelTextured() */
            .setSpecularColor(0x99ff99_rgbf);
    }

    /* Using default (white) light colors to have the texture data visible
       better */
    shader.setLightPositions({{-3.0f, -3.0f, 0.0f},
                              { 3.0f, -3.0f, 0.0f}})
        .setTransformationMatrix(
            Matrix4::translation(Vector3::zAxis(-2.15f))*
            Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf))
        /** @todo use normalMatrix() instead */
        .setNormalMatrix((Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf)).rotationScaling())
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f));

    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* SwiftShader has few rounding errors at the edges (giving a large max
       error), but that's basically it. */
    const Float maxThreshold = 210.4f, meanThreshold = 0.126f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's a bit worse */
    const Float maxThreshold = 210.4f, meanThreshold = 3.434f;
    #endif
    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Directory::join({SHADERS_TEST_DIR, "PhongTestFiles", data.expected}),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

void PhongGLTest::renderTexturedNormal() {
    auto&& data = RenderTexturedNormalData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer);

    GL::Texture2D normal;
    Containers::Optional<Trade::ImageData2D> image;
    CORRADE_VERIFY(importer->openFile(Utility::Directory::join(SHADERS_TEST_DIR, "TestFiles/normal-texture.tga")) && (image = importer->image2D(0)));
    normal.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, TextureFormatRGB, image->size())
        .setSubImage(0, {}, *image);

    GL::Mesh plane = MeshTools::compile(Primitives::planeSolid( Primitives::PlaneTextureCoords::Generate));

    /* Add hardcoded tangents */
    /** @todo remove once MeshData is sane */
    GL::Buffer tangents;
    tangents.setData(Containers::Array<Vector3>{Containers::DirectInit, 4, Vector3::xAxis()});
    plane.addVertexBuffer(std::move(tangents), 0, Shaders::Phong::Tangent{});

    /* Rotating the view a few times (together with light positions). If the
       tangent transformation in the shader is correct, it should result in
       exactly the same images. */
    Phong shader{Phong::Flag::NormalTexture, 2};
    shader.setLightPositions({
            Matrix4::rotationZ(data.rotation).transformPoint({-3.0f, -3.0f, 0.0f}),
            Matrix4::rotationZ(data.rotation).transformPoint({ 3.0f, -3.0f, 0.0f})})
        .setTransformationMatrix(Matrix4::translation(Vector3::zAxis(-2.35f))*
            Matrix4::rotationZ(data.rotation)*
            Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf))
        /** @todo use normalMatrix() instead */
        .setNormalMatrix((Matrix4::rotationZ(data.rotation)*
            Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf)).rotationScaling())
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f))
        .setDiffuseColor(0x999999_rgbf);

    if(data.multiBind)
        shader.bindTextures(nullptr, nullptr, nullptr, &normal);
    else
        shader.bindNormalTexture(normal);

    plane.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D actual = _framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm});
    Containers::StridedArrayView2D<Color3ub> pixels =
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(actual.pixels<Color4ub>());

    /* Rotate pixels back to upright position so we can compare with the 0°
       file and ensure the tangent calculation is transformation invariant */
    if(data.rotation == -90.0_degf)
        pixels = pixels.flipped<0>().transposed<0, 1>();
    else if(data.rotation == 90.0_degf)
        pixels = pixels.flipped<1>().transposed<0, 1>();
    else CORRADE_COMPARE(data.rotation, 0.0_degf);

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* One pixel in the center didn't survive the transformation. But that's
       okay. Due to the density of the normal map, SwiftShader has an overally
       consistent off-by-a-bit error. */
    const Float maxThreshold = 24.0f, meanThreshold = 0.3421f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
    const Float maxThreshold = 24.0f, meanThreshold = 3.017f;
    #endif
    CORRADE_COMPARE_WITH(pixels,
        Utility::Directory::join(SHADERS_TEST_DIR, "PhongTestFiles/textured-normal.tga"),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

void PhongGLTest::renderShininess() {
    auto&& data = RenderShininessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    GL::Mesh sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32));

    Phong shader;
    shader.setLightPosition({-3.0f, -3.0f, 0.0f})
        .setDiffuseColor(0xff3333_rgbf)
        .setSpecularColor(data.specular)
        .setShininess(data.shininess)
        .setTransformationMatrix(Matrix4::translation(Vector3::zAxis(-2.15f)))
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f));

    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    {
        #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
        /* SwiftShader has some minor rounding differences (max = 1.67). ARM
           Mali G71 has bigger rounding differences. */
        const Float maxThreshold = 12.0f, meanThreshold = 0.043f;
        #else
        /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
        const Float maxThreshold = 16.667f, meanThreshold = 2.583f;
        #endif
        #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
        CORRADE_EXPECT_FAIL_IF(data.shininess <= 0.0011f && (GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader),
            "SwiftShader has a much larger ring for the owerflown shininess.");
        #endif
        #if defined(CORRADE_TARGET_ANDROID) && defined(MAGNUM_TARGET_GLES2)
        CORRADE_EXPECT_FAIL_IF(data.shininess == 0.0f && (GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::ArmMali),
            "ARM Mali has a much larger ring for the owerflown shininess when it's exactly 0.");
        #endif
        CORRADE_COMPARE_WITH(
            /* Dropping the alpha channel, as it's always 1.0 */
            Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
            Utility::Directory::join({SHADERS_TEST_DIR, "PhongTestFiles", data.expected}),
            (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
    }

    /* Test the special overflow results as well */
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    if((data.shininess <= 0.0011f && (GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader))
        #if defined(CORRADE_TARGET_ANDROID) && defined(MAGNUM_TARGET_GLES2)
        || (data.shininess == 0.0f && (GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::ArmMali))
        #endif
    ) {
        CORRADE_COMPARE_WITH(
            /* Dropping the alpha channel, as it's always 1.0 */
            Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
            Utility::Directory::join({SHADERS_TEST_DIR, "PhongTestFiles", "shininess0-overflow.tga"}),
            /* The threshold = 0.001 case has a slight reddish tone on
               SwiftShader; ARM Mali has one pixel off */
            (DebugTools::CompareImageToFile{_manager, 255.0f, 1.475f}));
    }
    #endif
}

void PhongGLTest::renderAlphaSetup() {
    renderSetup();
    if(RenderAlphaData[testCaseInstanceId()].blending)
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add);
}

void PhongGLTest::renderAlphaTeardown() {
    if(RenderAlphaData[testCaseInstanceId()].blending)
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
    renderTeardown();
}

void PhongGLTest::renderAlpha() {
    auto&& data = RenderAlphaData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_manager.loadState("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.loadState("TgaImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / TgaImageImporter plugins not found.");

    Containers::Optional<Trade::ImageData2D> image;
    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer);

    GL::Texture2D ambient;
    CORRADE_VERIFY(importer->openFile(Utility::Directory::join({SHADERS_TEST_DIR, "TestFiles", data.ambientTexture})) && (image = importer->image2D(0)));
    ambient.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, TextureFormatRGBA, image->size())
        .setSubImage(0, {}, *image);

    GL::Texture2D diffuse;
    CORRADE_VERIFY(importer->openFile(Utility::Directory::join({SHADERS_TEST_DIR, "TestFiles", data.diffuseTexture})) && (image = importer->image2D(0)));
    diffuse.setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);

    /* In some instances the diffuse texture is just three-component, handle
       that properly */
    if(image->format() == PixelFormat::RGBA8Unorm)
        diffuse.setStorage(1, TextureFormatRGBA, image->size());
    else {
        CORRADE_COMPARE(image->format(), PixelFormat::RGB8Unorm);
        diffuse.setStorage(1, TextureFormatRGB, image->size());
    }
    diffuse.setSubImage(0, {}, *image);

    MAGNUM_VERIFY_NO_GL_ERROR();

    GL::Mesh sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32,
        Primitives::UVSphereTextureCoords::Generate));

    Phong shader{data.flags, 2};
    shader.setLightPositions({{-3.0f, -3.0f, 0.0f},
                              { 3.0f, -3.0f, 0.0f}})
        .setTransformationMatrix(
            Matrix4::translation(Vector3::zAxis(-2.15f))*
            Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf))
        /** @todo use normalMatrix() instead */
        .setNormalMatrix((Matrix4::rotationY(-15.0_degf)*
            Matrix4::rotationX(15.0_degf)).rotationScaling())
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 10.0f))
        .setAmbientColor(data.ambientColor)
        .setDiffuseColor(data.diffuseColor)
        .setSpecularColor(0xffffff00_rgbaf)
        .bindTextures(&ambient, &diffuse, nullptr, nullptr);

    if(data.flags & Phong::Flag::AlphaMask)
        shader.setAlphaMask(data.threshold);

    /* For proper Z order draw back faces first and then front faces */
    GL::Renderer::setFaceCullingMode(GL::Renderer::PolygonFacing::Front);
    sphere.draw(shader);
    GL::Renderer::setFaceCullingMode(GL::Renderer::PolygonFacing::Back);
    sphere.draw(shader);

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    /* In some cases (separate vs combined alpha) there are off-by-one errors.
       That's okay, as we have only 8bit texture precision. SwiftShader has
       additionally a few minor rounding errors at the edges. */
    const Float maxThreshold = 172.667f, meanThreshold = 0.171f;
    #else
    /* WebGL 1 doesn't have 8bit renderbuffer storage, so it's way worse */
    const Float maxThreshold = 172.667f, meanThreshold = 4.736f;
    #endif
    CORRADE_COMPARE_WITH(
        /* Dropping the alpha channel, as it's always 1.0 */
        Containers::arrayCast<Color3ub>(_framebuffer.read(_framebuffer.viewport(), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()),
        Utility::Directory::join(SHADERS_TEST_DIR, data.expected),
        (DebugTools::CompareImageToFile{_manager, maxThreshold, meanThreshold}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Shaders::Test::PhongGLTest)
