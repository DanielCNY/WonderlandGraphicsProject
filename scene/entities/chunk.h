#ifndef CHUNK_H
#define CHUNK_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "ground.h"
#include "entities/static_model.h"

class Chunk {
public:
    static constexpr int SIZE = 500;

    Chunk(int x, int z);
    void initialize();
    void render(const glm::mat4& viewProjectionMatrix);
    bool isVisible(const glm::vec3& cameraPos) const;

private:
    int chunkX, chunkZ;
    GroundPlane ground;
    StaticModel tree;
};

#endif