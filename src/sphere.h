#ifndef SPHERE_H
#define SPHERE_H

#include <cmath>
#include <iostream>
#include <vector>

// Struct to hold sphere data
struct Sphere {
    float* vertices;
    float* normals;
    float* texCoords;
    unsigned int* indices;
    unsigned int vertexCount;
    unsigned int indexCount;

    float radius;
    int sectorCount;
    int stackCount;


    // Constructor
    Sphere(float radius, int sectorCount, int stackCount, float aspectRatio = 1.0f) {
        this->radius = radius;
        this->sectorCount = sectorCount;
        this->stackCount = stackCount;

        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
        float s, t;                                      // vertex texCoord
        vertexCount = 0;
        int normalCount = 0;
        int texCoordCount = 0;
        indexCount = 0;

        float sectorStep = 2 * M_PI / sectorCount;
        float stackStep = M_PI / stackCount;
        float sectorAngle, stackAngle;

        // Allocate memory for vertices, normals, texCoords, and indices arrays
        vertices = new float[(sectorCount + 1) * (stackCount + 1) * 3];
        normals = new float[(sectorCount + 1) * (stackCount + 1) * 3];
        texCoords = new float[(sectorCount + 1) * (stackCount + 1) * 2];
        indices = new unsigned int[sectorCount * stackCount * 6];

        for (int i = 0; i <= stackCount; ++i) {
            stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);               // r * cos(u)
            z = radius * sinf(stackAngle);                // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for (int j = 0; j <= sectorCount; ++j) {
                sectorAngle = j * sectorStep;           // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);            // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);            // r * cos(u) * sin(v) 
                if (aspectRatio >= 1.0f) {
                    x /= aspectRatio;
                }
                if (aspectRatio < 1.0f) {
                    y *= aspectRatio;
                }
                vertices[vertexCount++] = x;
                vertices[vertexCount++] = y;
                vertices[vertexCount++] = z;

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                normals[normalCount++] = nx;
                normals[normalCount++] = ny;
                normals[normalCount++] = nz;

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / sectorCount;
                t = (float)i / stackCount;
                texCoords[texCoordCount++] = s;
                texCoords[texCoordCount++] = t;
            }
        }

        // Generate indices
        for (int i = 0; i < stackCount; ++i) {
            int k1 = i * (sectorCount + 1);     // beginning of current stack
            int k2 = k1 + sectorCount + 1;      // beginning of next stack

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                indices[indexCount++] = k1;
                indices[indexCount++] = k2;
                indices[indexCount++] = k1 + 1;

                // k1+1 => k2 => k2+1
                indices[indexCount++] = k1 + 1;
                indices[indexCount++] = k2;
                indices[indexCount++] = k2 + 1;
            }
        }
    }

    // Destructor
    ~Sphere() {
        delete[] vertices;
        delete[] normals;
        delete[] texCoords;
        delete[] indices;
    }

    // Function to get the vertices
    float* getVertices() {
        return vertices;
    }

    // Function to get the normals
    float* getNormals() {
        return normals;
    }

    // Function to get the texCoords
    float* getTexCoords() {
        return texCoords;
    }

    // Function to get the indices
    unsigned int* getIndices() {
        return indices;
    }

    // Function to get the vertex count
    unsigned int getVertexCount() {
        return vertexCount;
    }

    // Function to get the index count
    unsigned int getIndexCount() {
        return indexCount;
    }
};

#endif // SPHERE_H
