/**
 *
 *      \file Mesh.cpp
 *
 *      Clean everything XXX
 *
 */

#include <cstdio>
#include <cassert>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <tools/gltools.hpp>
#include "Mesh.h"


void Mesh::destroy()
{
    m_vertexBuffer.destroy();
}

/** PLANE MESH ----------------------------------------- */

void PlaneMesh::create()
{
    /**
     *  todo : use indexed tristrip instead
     */

    assert(!m_bInitialized);
    m_bInitialized = true;

    const float SIZE = m_size; //
    const int RES = static_cast<int>(m_res); //  

    m_count = 3 * 2 * (RES*RES);

    std::vector<glm::vec3> &positions = m_vertexBuffer.getPosition();
    std::vector<glm::vec3> &normals = m_vertexBuffer.getNormal();
    std::vector<glm::vec2> &texCoords = m_vertexBuffer.getTexcoord();

    positions.resize(m_count);
    normals.resize(m_count, glm::vec3(0.0f, 1.0f, 0.0f));
    texCoords.resize(m_count);

    glm::vec3 *pPos = &(positions[0]);
    glm::vec2 *pUV = &(texCoords[0]);

	float UVScale = m_UVScale;

    const float Delta = 1.0f / float(RES);

    for(int j = 0; j < RES; ++j)
    {
        for(int i = 0; i < RES; ++i)
        {
            glm::vec4 vInd = Delta * glm::vec4(i, j, i + 1, j + 1);

            *pUV = glm::vec2(vInd.x, vInd.y);
            *pPos = SIZE * glm::vec3(pUV->x - 0.5f, 0.0f, pUV->y - 0.5f);
			*pUV *= UVScale;
            ++pPos; ++pUV;

            *pUV = glm::vec2(vInd.x, vInd.w);
            *pPos = SIZE * glm::vec3(pUV->x - 0.5f, 0.0f, pUV->y - 0.5f);
			*pUV *= UVScale;
            ++pPos; ++pUV;

            *pUV = glm::vec2(vInd.z, vInd.y);
            *pPos = SIZE * glm::vec3(pUV->x - 0.5f, 0.0f, pUV->y - 0.5f);
			*pUV *= UVScale;
            ++pPos; ++pUV;


            *pUV = pUV[-1];
            *pPos = pPos[-1];
            ++pPos; ++pUV;

            *pUV = pUV[-3];
            *pPos = pPos[-3];
            ++pPos; ++pUV;

            *pUV = glm::vec2(vInd.z, vInd.w);
            *pPos = SIZE * glm::vec3(pUV->x - 0.5f, 0.0f, pUV->y - 0.5f);
			*pUV *= UVScale;
            ++pPos; ++pUV;
        }
    }

    for (auto& coord : texCoords)
        coord.g = 1 - coord.g;

    m_vertexBuffer.initialize();
    m_vertexBuffer.complete(GL_STATIC_DRAW);
    m_vertexBuffer.cleanData();

    CHECKGLERROR();
}

void PlaneMesh::draw() const
{
    assert(m_bInitialized);

    m_vertexBuffer.enable();
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    m_vertexBuffer.disable();

    CHECKGLERROR();
}



/** SPHERE MESH ----------------------------------------- */

void SphereMesh::create()
{
    assert(!m_bInitialized);
    m_bInitialized = true;


    const float RADIUS = m_radius; //

    m_count = 2 * m_meshResolution*(m_meshResolution + 2);

    std::vector<glm::vec3> &positions = m_vertexBuffer.getPosition();
    std::vector<glm::vec3> &normals = m_vertexBuffer.getNormal();
    std::vector<glm::vec2> &texCoords = m_vertexBuffer.getTexcoord();

    positions.resize(m_count);
    normals.resize(m_count);
    texCoords.resize(m_count);

    glm::vec3 *pPos = &(positions[0]);
    glm::vec3 *pNor = &(normals[0]);
    glm::vec2 *pUV = &(texCoords[0]);


    float theta2, phi;    // next theta angle, phi angle
    float ct, st;         // cos(theta), sin(theta)
    float ct2, st2;       // cos(next theta), sin(next theta)
    float cp, sp;         // cos(phi), sin(phi)


    const float TwoPI = 2.0f*(float)M_PI;
    const float Delta = 1.0f / float(m_meshResolution);

    ct2 = 0.0f; st2 = -1.0f;

    /* Create the sphere from bottom to top (like a spiral) as a tristrip */
    for(int j = 0; j < m_meshResolution; ++j)
    {
        ct = ct2;
        st = st2;

        theta2 = ((j + 1) * Delta - 0.5f) * (float)M_PI;
        ct2 = cos(theta2);
        st2 = sin(theta2);

        pNor->x = ct;
        pNor->y = st;
        pNor->z = 0.0f;
        *pPos = RADIUS * (*pNor);
        pUV->x = 0.0f;
        pUV->y = j * Delta;
        ++pPos; ++pNor; ++pUV;

        for(int i = 0; i < m_meshResolution + 1; ++i)
        {
            phi = TwoPI * i * Delta;
            cp = cos(phi);
            sp = sin(phi);

            pNor->x = ct2 * cp;
            pNor->y = st2;
            pNor->z = ct2 * sp;
            *pPos = RADIUS * (*pNor);
            pUV->x = i * Delta;
            pUV->y = (j + 1) * Delta;
            ++pPos; ++pNor; ++pUV;

            pNor->x = ct * cp;
            pNor->y = st;
            pNor->z = ct * sp;
            *pPos = RADIUS * (*pNor);
            pUV->x = i * Delta;
            pUV->y = j * Delta;
            ++pPos; ++pNor; ++pUV;
        }

        pNor->x = ct2;
        pNor->y = st2;
        pNor->z = 0.0f;
        *pPos = RADIUS * (*pNor);
        pUV->x = 1.0f;
        pUV->y = 1.0f;
        ++pPos; ++pNor; ++pUV;
    }

    //-------------------------


    m_vertexBuffer.initialize();
    m_vertexBuffer.complete(GL_STATIC_DRAW);
    m_vertexBuffer.cleanData();

    CHECKGLERROR();
}

void SphereMesh::draw() const
{
    assert(m_bInitialized);

    m_vertexBuffer.enable();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, m_count);
    m_vertexBuffer.disable();

    CHECKGLERROR();
}



/** CONE MESH ----------------------------------------- */

void ConeMesh::create()
{
    assert(!m_bInitialized);
    m_bInitialized = true;

    const float RADIUS = 1.0f; //
    const float HEIGHT = 1.0f; //
    const unsigned int RES = 24;

    m_count = 2 * 3 * (RES);

    std::vector<glm::vec3> &positions = m_vertexBuffer.getPosition();
    std::vector<glm::vec3> &normals = m_vertexBuffer.getNormal();
    std::vector<glm::vec2> &texCoords = m_vertexBuffer.getTexcoord();
    // Note : not sure for the uv coords

    positions.resize(m_count);
    normals.resize(m_count);
    texCoords.resize(m_count);

    glm::vec3 *pPos = &(positions[0]);
    glm::vec3 *pNor = &(normals[0]);
    glm::vec2 *pUV = &(texCoords[0]);


    std::vector<glm::vec3> baseVertex(RES);

    float theta2;     // next theta angle
    float ct, st;     // cos(theta), sin(theta)
    float ct2, st2;   // cos(next theta), sin(next theta)  

    const float TwoPI = 2.0f * (float)M_PI;
    const float Delta = 1.0f / float(RES);

    ct2 = -1.0f; st2 = 0.0f;

    size_t i;

    // Structure
    for(i = 0; i < RES; ++i)
    {
        ct = ct2;
        st = st2;

        theta2 = ((i + 1) * Delta - 0.5f) * TwoPI;
        ct2 = cos(theta2);
        st2 = sin(theta2);

        *pUV = glm::vec2(i*Delta, 1.0f);
        *pPos = glm::vec3(0.0f);
        ++pPos; ++pUV;

        *pUV = glm::vec2(i*Delta, 0.0f);
        *pPos = glm::vec3(RADIUS*ct, RADIUS*st, -HEIGHT);
        ++pPos; ++pUV;

        *pUV = glm::vec2((i + 1)*Delta, 0.0f);
        *pPos = glm::vec3(RADIUS*ct2, RADIUS*st2, -HEIGHT);
        baseVertex[i] = *pPos;
        ++pPos; ++pUV;

        pNor[0] = pNor[1] = pNor[2] = glm::normalize(glm::cross(pPos[-2], pPos[-1]));
        pNor += 3;
    }

    // Adding the first one to loop
    baseVertex.push_back(baseVertex[0]);

    // Base
    for(i = 0; i < baseVertex.size() - 1u; ++i)
    {
        *pNor = glm::vec3(0.0f, 0.0f, -1.0f);
        *pUV = glm::vec2(0.5f, 0.0f); //
        *pPos = glm::vec3(0.0f, 0.0f, -HEIGHT);
        ++pPos; ++pNor; ++pUV;

        *pNor = glm::vec3(0.0f, 0.0f, -1.0f);
        *pUV = glm::vec2((i + 1)*Delta, 0.0f); //
        *pPos = baseVertex[i + 1];
        ++pPos; ++pNor; ++pUV;

        *pNor = glm::vec3(0.0f, 0.0f, -1.0f);
        *pUV = glm::vec2((i)*Delta, 0.0f); //
        *pPos = baseVertex[i];
        ++pPos; ++pNor; ++pUV;
    }


    //-------------------------


    m_vertexBuffer.initialize();
    m_vertexBuffer.complete(GL_STATIC_DRAW);
    m_vertexBuffer.cleanData();

    CHECKGLERROR();
}

void ConeMesh::draw() const
{
    assert(m_bInitialized);

    m_vertexBuffer.enable();
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    m_vertexBuffer.disable();

    CHECKGLERROR();
}



/** CUBE MESH ----------------------------------------- */

void CubeMesh::create()
{
    /// not optimized at all, better with indices

    assert(!m_bInitialized);
    m_bInitialized = true;

    /**
     *  note : for cubemap, texcoord are object space position coords.
     */

    m_count = 2u * 3u * 6u;

    std::vector<glm::vec3> &positions = m_vertexBuffer.getPosition();
    std::vector<glm::vec3> &normals = m_vertexBuffer.getNormal();
    std::vector<glm::vec2> &coords = m_vertexBuffer.getTexcoord();

    positions.resize(m_count);
    normals.resize(m_count);
    coords.resize(m_count);

#define PX  0u
#define NX  6u
#define PY  12u
#define NY  18u
#define PZ  24u
#define NZ  30u

    glm::vec3 default_normals[] =
    {
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)
    };

    /// POSITIVE-X
    positions[PX + 0u] = positions[PX + 5u] = glm::vec3(1.0f, -1.0f, 1.0f);
    positions[PX + 1u] = glm::vec3(1.0f, -1.0f, -1.0f);
    positions[PX + 2u] = positions[PX + 3u] = glm::vec3(1.0f, 1.0f, -1.0f);
    positions[PX + 4u] = glm::vec3(1.0f, 1.0f, 1.0f);
    normals[PX + 0u] = normals[PX + 1u] = normals[PX + 2u] =
        normals[PX + 3u] = normals[PX + 4u] = normals[PX + 5u] = default_normals[0u];

    /// NEGATIVE-X
    positions[NX + 0u] = positions[NX + 5u] = glm::vec3(-1.0f, -1.0f, -1.0f);
    positions[NX + 1u] = glm::vec3(-1.0f, -1.0f, 1.0f);
    positions[NX + 2u] = positions[NX + 3u] = glm::vec3(-1.0f, 1.0f, 1.0f);
    positions[NX + 4u] = glm::vec3(-1.0f, 1.0f, -1.0f);
    normals[NX + 0u] = normals[NX + 1u] = normals[NX + 2u] =
        normals[NX + 3u] = normals[NX + 4u] = normals[NX + 5u] = -default_normals[0u];

    /// POSITIVE-Y
    positions[PY + 0u] = positions[PY + 5u] = glm::vec3(-1.0f, 1.0f, 1.0f);
    positions[PY + 1u] = glm::vec3(1.0f, 1.0f, 1.0f);
    positions[PY + 2u] = positions[PY + 3u] = glm::vec3(1.0f, 1.0f, -1.0f);
    positions[PY + 4u] = glm::vec3(-1.0f, 1.0f, -1.0f);
    normals[PY + 0u] = normals[PY + 1u] = normals[PY + 2u] =
        normals[PY + 3u] = normals[PY + 4u] = normals[PY + 5u] = default_normals[1u];

    /// NEGATIVE-Y
    positions[NY + 0u] = positions[NY + 5u] = glm::vec3(-1.0f, -1.0f, -1.0f);
    positions[NY + 1u] = glm::vec3(1.0f, -1.0f, -1.0f);
    positions[NY + 2u] = positions[NY + 3u] = glm::vec3(1.0f, -1.0f, 1.0f);
    positions[NY + 4u] = glm::vec3(-1.0f, -1.0f, 1.0f);
    normals[NY + 0u] = normals[NY + 1u] = normals[NY + 2u] =
        normals[NY + 3u] = normals[NY + 4u] = normals[NY + 5u] = -default_normals[1u];

    /// POSITIVE-Z
    positions[PZ + 0u] = positions[PZ + 5u] = glm::vec3(-1.0f, -1.0f, 1.0f);
    positions[PZ + 1u] = glm::vec3(1.0f, -1.0f, 1.0f);
    positions[PZ + 2u] = positions[PZ + 3u] = glm::vec3(1.0f, 1.0f, 1.0f);
    positions[PZ + 4u] = glm::vec3(-1.0f, 1.0f, 1.0f);
    normals[PZ + 0u] = normals[PZ + 1u] = normals[PZ + 2u] =
        normals[PZ + 3u] = normals[PZ + 4u] = normals[PZ + 5u] = default_normals[2u];

    /// NEGATIVE-Z
    positions[NZ + 0u] = positions[NZ + 5u] = glm::vec3(1.0f, -1.0f, -1.0f);
    positions[NZ + 1u] = glm::vec3(-1.0f, -1.0f, -1.0f);
    positions[NZ + 2u] = positions[NZ + 3u] = glm::vec3(-1.0f, 1.0f, -1.0f);
    positions[NZ + 4u] = glm::vec3(1.0f, 1.0f, -1.0f);
    normals[NZ + 0u] = normals[NZ + 1u] = normals[NZ + 2u] =
        normals[NZ + 3u] = normals[NZ + 4u] = normals[NZ + 5u] = -default_normals[2u];

    coords[PX + 0u] = coords[NX + 0u] = coords[PY + 0u] = coords[NY + 0u] = coords[PZ + 0u] = coords[NZ + 0u] = glm::vec2(1.0f, 0.f);
    coords[PX + 1u] = coords[NX + 1u] = coords[PY + 1u] = coords[NY + 1u] = coords[PZ + 1u] = coords[NZ + 1u] = glm::vec2(0.0f, 0.f);
    coords[PX + 2u] = coords[NX + 2u] = coords[PY + 2u] = coords[NY + 2u] = coords[PZ + 2u] = coords[NZ + 2u] = glm::vec2(0.0f, 1.f);
    coords[PX + 3u] = coords[NX + 3u] = coords[PY + 3u] = coords[NY + 3u] = coords[PZ + 3u] = coords[NZ + 3u] = glm::vec2(0.0f, 1.f);
    coords[PX + 4u] = coords[NX + 4u] = coords[PY + 4u] = coords[NY + 4u] = coords[PZ + 4u] = coords[NZ + 4u] = glm::vec2(1.0f, 1.f);
    coords[PX + 5u] = coords[NX + 5u] = coords[PY + 5u] = coords[NY + 5u] = coords[PZ + 5u] = coords[NZ + 5u] = glm::vec2(1.0f, 0.f);

    //-------------------------

    m_vertexBuffer.initialize();
    m_vertexBuffer.complete(GL_STATIC_DRAW);
    m_vertexBuffer.cleanData();

    CHECKGLERROR();
}

void CubeMesh::draw() const
{
    assert(m_bInitialized);

    m_vertexBuffer.enable();
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    m_vertexBuffer.disable();

    CHECKGLERROR();
}

/** FULL SCREEN TRIANGLE MESH -------------------------- */

void FullscreenTriangleMesh::create()
{
    assert(!m_bInitialized);
    m_bInitialized = true;

    m_count = 3;

    std::vector<glm::vec3> &positions = m_vertexBuffer.getPosition();
    std::vector<glm::vec3> &normals = m_vertexBuffer.getNormal();
    std::vector<glm::vec2> &texCoords = m_vertexBuffer.getTexcoord();

    positions.resize(m_count);
    normals.resize(m_count, glm::vec3(0.0f, 0.0f, -1.0f));
    texCoords.resize(m_count);

    positions[0] = glm::vec3(-1, -1, 0);
    positions[1] = glm::vec3(3, -1, 0);
    positions[2] = glm::vec3(-1, 3, 0);

    texCoords[0] = glm::vec2(0, 0);
    texCoords[1] = glm::vec2(2, 0);
    texCoords[2] = glm::vec2(0, 2);

    m_vertexBuffer.initialize();
    m_vertexBuffer.complete(GL_STATIC_DRAW);
    m_vertexBuffer.cleanData();

    CHECKGLERROR();
}


void FullscreenTriangleMesh::draw() const
{
    assert(m_bInitialized);

    m_vertexBuffer.enable();
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    m_vertexBuffer.disable();

    CHECKGLERROR();
}
