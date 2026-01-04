#include "chunk.h"

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
}

void Chunk::initialize() {
    ground.initialize();
}

void Chunk::render(const glm::mat4& viewProjectionMatrix) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix,glm::vec3(chunkX * SIZE, 0.0f, chunkZ * SIZE));

    glm::mat4 mvp = viewProjectionMatrix * modelMatrix;
    ground.render(mvp);
}

bool Chunk::isVisible(const glm::vec3& cameraPos) const {
    float chunkCenterX = chunkX * SIZE + SIZE/2.0f;
    float chunkCenterZ = chunkZ * SIZE + SIZE/2.0f;

    float dx = chunkCenterX - cameraPos.x;
    float dz = chunkCenterZ - cameraPos.z;
    float distance = sqrt(dx * dx + dz * dz);

    return distance < 3000.0f;
}