//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"

using namespace angle;

template <typename IndexType, GLenum IndexTypeName>
class IndexedPointsTest : public ANGLETest
{
  protected:
    IndexedPointsTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    float getIndexPositionX(size_t idx) { return (idx == 0 || idx == 3) ? -0.5f : 0.5f; }

    float getIndexPositionY(size_t idx) { return (idx == 2 || idx == 3) ? -0.5f : 0.5f; }

    void SetUp() override
    {
        ANGLETest::SetUp();

        constexpr char kVS[] = R"(precision highp float;
attribute vec2 position;

void main() {
    gl_PointSize = 5.0;
    gl_Position  = vec4(position, 0.0, 1.0);
})";

        constexpr char kFS[] = R"(precision highp float;
void main()
{
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";

        mProgram = CompileProgram(kVS, kFS);
        ASSERT_NE(0u, mProgram);

        constexpr char kVS2[] = R"(precision highp float;
attribute vec2 position;
attribute vec4 color;
varying vec4 vcolor;

void main() {
    gl_PointSize = 5.0;
    gl_Position  = vec4(position, 0.0, 1.0);
    vcolor       = color;
})";

        constexpr char kFS2[] = R"(precision highp float;
varying vec4 vcolor;

void main()
{
    gl_FragColor = vec4(vcolor.xyz, 1.0);
})";

        mVertexWithColorBufferProgram = CompileProgram(kVS2, kFS2);
        ASSERT_NE(0u, mVertexWithColorBufferProgram);

        // Construct a vertex buffer of position values and color values
        // contained in a single structure
        const float verticesWithColor[] = {
            getIndexPositionX(0), getIndexPositionY(0), 0.0f, 1.0f, 0.0f,
            getIndexPositionX(2), getIndexPositionY(2), 0.0f, 1.0f, 0.0f,
            getIndexPositionX(1), getIndexPositionY(1), 0.0f, 1.0f, 0.0f,
            getIndexPositionX(3), getIndexPositionY(3), 0.0f, 1.0f, 0.0f,
        };

        glGenBuffers(1, &mVertexWithColorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexWithColorBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesWithColor), &verticesWithColor[0],
                     GL_STATIC_DRAW);

        // Construct a vertex buffer of position values only
        const GLfloat vertices[] = {
            getIndexPositionX(0), getIndexPositionY(0), getIndexPositionX(2), getIndexPositionY(2),
            getIndexPositionX(1), getIndexPositionY(1), getIndexPositionX(3), getIndexPositionY(3),
        };
        glGenBuffers(1, &mVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        // The indices buffer is shared between both variations of tests
        const IndexType indices[] = {0, 2, 1, 3};
        glGenBuffers(1, &mIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);
    }

    void TearDown() override
    {
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteBuffers(1, &mIndexBuffer);
        glDeleteProgram(mProgram);

        glDeleteBuffers(1, &mVertexWithColorBuffer);
        glDeleteProgram(mVertexWithColorBufferProgram);
        ANGLETest::TearDown();
    }

    void runTest(GLuint firstIndex, bool useVertexBufferWithColor = false)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLint viewportSize[4];
        glGetIntegerv(GL_VIEWPORT, viewportSize);

        // Choose appropriate program to apply for the test
        GLuint program = useVertexBufferWithColor ? mVertexWithColorBufferProgram : mProgram;

        if (useVertexBufferWithColor)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mVertexWithColorBuffer);
            GLint vertexLocation = glGetAttribLocation(program, "position");
            glVertexAttribPointer(vertexLocation, 2, GL_FLOAT, GL_FALSE,
                                  static_cast<const GLsizei>(VertexWithColorSize), 0);
            glEnableVertexAttribArray(vertexLocation);

            GLint vertexColorLocation = glGetAttribLocation(program, "color");
            glVertexAttribPointer(vertexColorLocation, 3, GL_FLOAT, GL_FALSE,
                                  static_cast<const GLsizei>(VertexWithColorSize),
                                  (void *)((sizeof(float) * 2)));
            glEnableVertexAttribArray(vertexColorLocation);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
            GLint vertexLocation = glGetAttribLocation(program, "position");
            glVertexAttribPointer(vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(vertexLocation);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glUseProgram(program);

        glDrawElements(GL_POINTS, mPointCount - firstIndex, IndexTypeName,
                       reinterpret_cast<void *>(firstIndex * sizeof(IndexType)));

        for (size_t i = 0; i < mPointCount; i++)
        {
            GLuint x =
                static_cast<GLuint>(viewportSize[0] + (getIndexPositionX(i) * 0.5f + 0.5f) *
                                                          (viewportSize[2] - viewportSize[0]));
            GLuint y =
                static_cast<GLuint>(viewportSize[1] + (getIndexPositionY(i) * 0.5f + 0.5f) *
                                                          (viewportSize[3] - viewportSize[1]));

            if (i < firstIndex)
            {
                EXPECT_PIXEL_EQ(x, y, 0, 0, 0, 255);
            }
            else
            {
                if (useVertexBufferWithColor)
                {
                    // Pixel data is assumed to be GREEN
                    EXPECT_PIXEL_EQ(x, y, 0, 255, 0, 255);
                }
                else
                {
                    // Pixel data is assumed to be RED
                    EXPECT_PIXEL_EQ(x, y, 255, 0, 0, 255);
                }
            }
        }
        swapBuffers();
    }

    GLuint mProgram;
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;

    GLuint mVertexWithColorBufferProgram;
    GLuint mVertexWithColorBuffer;

    static const GLuint mPointCount = 4;

  private:
    const size_t VertexWithColorSize = sizeof(float) * 5;
};

typedef IndexedPointsTest<GLubyte, GL_UNSIGNED_BYTE> IndexedPointsTestUByte;

TEST_P(IndexedPointsTestUByte, UnsignedByteOffset0)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(0);
}

TEST_P(IndexedPointsTestUByte, UnsignedByteOffset1)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(1);
}

TEST_P(IndexedPointsTestUByte, UnsignedByteOffset2)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(2);
}

TEST_P(IndexedPointsTestUByte, UnsignedByteOffset3)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(3);
}

TEST_P(IndexedPointsTestUByte, VertexWithColorUnsignedByteOffset0)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(0, true);
}

TEST_P(IndexedPointsTestUByte, VertexWithColorUnsignedByteOffset1)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(1, true);
}

TEST_P(IndexedPointsTestUByte, VertexWithColorUnsignedByteOffset2)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(2, true);
}

TEST_P(IndexedPointsTestUByte, VertexWithColorUnsignedByteOffset3)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(3, true);
}

typedef IndexedPointsTest<GLushort, GL_UNSIGNED_SHORT> IndexedPointsTestUShort;

TEST_P(IndexedPointsTestUShort, UnsignedShortOffset0)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(0);
}

TEST_P(IndexedPointsTestUShort, UnsignedShortOffset1)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(1);
}

TEST_P(IndexedPointsTestUShort, UnsignedShortOffset2)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(2);
}

TEST_P(IndexedPointsTestUShort, UnsignedShortOffset3)
{
    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(3);
}

TEST_P(IndexedPointsTestUShort, VertexWithColorUnsignedShortOffset0)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(0, true);
}

TEST_P(IndexedPointsTestUShort, VertexWithColorUnsignedShortOffset1)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(1, true);
}

TEST_P(IndexedPointsTestUShort, VertexWithColorUnsignedShortOffset2)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(2, true);
}

TEST_P(IndexedPointsTestUShort, VertexWithColorUnsignedShortOffset3)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(3, true);
}

TEST_P(IndexedPointsTestUShort, VertexWithColorUnsignedShortOffsetChangingIndices)
{
    // TODO(fjhenigman): Fix with buffer offset http://anglebug.com/2848
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAMD());

    // TODO(fjhenigman): Figure out why this fails on Ozone Intel.
    ANGLE_SKIP_TEST_IF(IsOzone() && IsIntel() && IsOpenGLES());

    // http://anglebug.com/2599: Fails on the 5x due to driver bug.
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsVulkan());

    runTest(3, true);
    runTest(1, true);
    runTest(0, true);
    runTest(2, true);
}

typedef IndexedPointsTest<GLuint, GL_UNSIGNED_INT> IndexedPointsTestUInt;

TEST_P(IndexedPointsTestUInt, UnsignedIntOffset0)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(0);
}

TEST_P(IndexedPointsTestUInt, UnsignedIntOffset1)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(1);
}

TEST_P(IndexedPointsTestUInt, UnsignedIntOffset2)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(2);
}

TEST_P(IndexedPointsTestUInt, UnsignedIntOffset3)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(3);
}

TEST_P(IndexedPointsTestUInt, VertexWithColorUnsignedIntOffset0)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(0, false);
}

TEST_P(IndexedPointsTestUInt, VertexWithColorUnsignedIntOffset1)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(1, false);
}

TEST_P(IndexedPointsTestUInt, VertexWithColorUnsignedIntOffset2)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(2, false);
}

TEST_P(IndexedPointsTestUInt, VertexWithColorUnsignedIntOffset3)
{
    if (getClientMajorVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(3, false);
}

// TODO(lucferron): Diagnose and fix the UByte tests below for Vulkan.
// http://anglebug.com/2646

// TODO(geofflang): Figure out why this test fails on Intel OpenGL
ANGLE_INSTANTIATE_TEST(IndexedPointsTestUByte,
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());
ANGLE_INSTANTIATE_TEST(IndexedPointsTestUShort,
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());
ANGLE_INSTANTIATE_TEST(IndexedPointsTestUInt,
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());