#ifndef CHUNK_H
#define CHUNK_H

#include "ground.h"
#include <glm/gtc/matrix_transform.hpp>

class Chunk {
public:
    Chunk(int chunkX, int chunkZ);
    void initialize();
    void render(const glm::mat4& viewProjectionMatrix);
    bool isVisible(const glm::vec3& cameraPos) const;

    int getChunkX() const { return chunkX; }
    int getChunkZ() const { return chunkZ; }

    static const int SIZE = 300;

private:
    int chunkX, chunkZ;
    GroundPlane ground;
};

#endif