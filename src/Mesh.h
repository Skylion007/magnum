#ifndef Magnum_Mesh_h
#define Magnum_Mesh_h
/*
    Copyright © 2010 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Magnum::Mesh
 */

#include <map>
#include <vector>
#include <set>

#include "Magnum.h"

namespace Magnum {

class Buffer;

/**
 * @brief Base class for managing non-indexed meshes
 *
 * @todo Support for normalized values (e.g. for color as char[4] passed to
 *      shader as floating-point vec4)
 */
class Mesh {
    DISABLE_COPY(Mesh)

    public:
        /** @brief Primitive type */
        enum Primitive {
            /**
             * Single points
             */
            Points = GL_POINTS,

            /**
             * Each pair of vertices defines a single line, lines aren't
             * connected together.
             */
            Lines = GL_LINES,

            /**
             * Polyline
             */
            LineStrip = GL_LINE_STRIP,

            /**
             * Polyline, last vertex is connected to first.
             */
            LineLoop = GL_LINE_LOOP,

            /**
             * Each three vertices define one triangle.
             */
            Triangles = GL_TRIANGLES,

            /**
             * First three vertices define first triangle, each following
             * vertex defines another triangle.
             */
            TriangleStrip = GL_TRIANGLE_STRIP,

            /**
             * First vertex is center, each following vertex is connected to
             * previous and center vertex.
             */
            TriangleFan = GL_TRIANGLE_FAN
        };

        /**
         * @brief Constructor
         * @param primitive     Primitive type
         * @param _count        Vertex count
         */
        inline Mesh(Primitive primitive, GLsizei _count): _primitive(primitive), count(_count), finalized(false) {}

        /**
         * @brief Destructor
         *
         * Deletes all associated buffers.
         */
        virtual ~Mesh();

        /** @brief Primitive type */
        inline Primitive primitive() const { return _primitive; }

        /**
         * @brief Add buffer
         * @param interleaved   If storing more than one attribute data in the
         *      buffer, the data of one attribute can be either kept together
         *      or interleaved with data for another attributes, so data for
         *      every vertex will be in one continuous place.
         *
         * Adds new buffer to the mesh. The buffer can be then filled with
         * Buffer::setData().
         */
        Buffer* addBuffer(bool interleaved);

        /**
         * @brief Bind attribute
         * @param buffer        Buffer where bind the attribute to (pointer
         *      returned by addBuffer())
         * @param attribute     Attribute
         *
         * Binds attribute of given type with given buffer. If the attribute is
         * already bound, given buffer isn't managed with this mesh (wasn't
         * initialized with addBuffer) or the mesh was already drawn, the
         * function does nothing.
         */
        template<class T> void bindAttribute(Buffer* buffer, GLuint attribute);

        /**
         * @brief Draw the mesh
         *
         * Binds attributes to buffers and draws the mesh. Expects an active
         * shader with all uniforms set.
         */
        virtual void draw();

    protected:
        struct Attribute {
            GLuint location;
            GLint size;
            GLenum type;
            GLsizei stride;
            const GLvoid* pointer;
        };

        /**
         * @brief Buffers with their attributes
         * @return Map of associated buffers, evey buffer has:
         * - boolean value which signalizes whether the buffer is interleaved
         * - list of bound attributes
         */
        inline const std::map<Buffer*, std::pair<bool, std::vector<Attribute> > >& buffers() { return _buffers; }

        /**
         * @brief List of all bound attributes
         *
         * List of all bound attributes bound with bindAttribute().
         */
        inline const std::set<GLuint>& attributes() { return _attributes; }

        /**
         * @brief Finalize the mesh
         *
         * Computes location and stride of each attribute in its buffer. After
         * this function is called, no new attribute can be bound.
         */
        void finalize();

    private:
        Primitive _primitive;
        GLsizei count;
        bool finalized;

        std::map<Buffer*, std::pair<bool, std::vector<Attribute> > > _buffers;
        std::set<GLuint> _attributes;

        void bindAttribute(Buffer* buffer, GLuint attribute, GLint size, GLenum type);

        GLsizei sizeOf(GLenum type);
};

template<> inline void Mesh::bindAttribute<GLbyte>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_BYTE);
}

template<> inline void Mesh::bindAttribute<GLubyte>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_UNSIGNED_BYTE);
}

template<> inline void Mesh::bindAttribute<GLshort>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_SHORT);
}

template<> inline void Mesh::bindAttribute<GLushort>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_UNSIGNED_SHORT);
}

template<> inline void Mesh::bindAttribute<GLint>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_INT);
}

template<> inline void Mesh::bindAttribute<GLuint>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_UNSIGNED_INT);
}

template<> inline void Mesh::bindAttribute<GLfloat>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_FLOAT);
}

template<> inline void Mesh::bindAttribute<GLdouble>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 1, GL_DOUBLE);
}

template<> inline void Mesh::bindAttribute<Vector3>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 3, GL_FLOAT);
}

template<> inline void Mesh::bindAttribute<Vector4>(Buffer* buffer, GLuint attribute) {
    bindAttribute(buffer, attribute, 4, GL_FLOAT);
}

}

#endif
