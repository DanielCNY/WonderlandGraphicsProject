#include "world_manager.h"
#include "../entities/chunk.h"
#include <cmath>
#include <iostream>

static const int CHUNK_RADIUS = 3;

WorldManager::~WorldManager() {
    chunks.clear();
}

int WorldManager::getChunkCoord(float worldCoord) {
    return static_cast<int>(std::round(worldCoord / Chunk::SIZE));
}

WorldManager::WorldManager() : centerChunkX(0), centerChunkZ(0), initialized(false) {
    std::cout << "WorldManager constructor" << std::endl;
}

void WorldManager::update(const glm::vec3& cameraPos) {
    int newCenterChunkX = getChunkCoord(cameraPos.x);
    int newCenterChunkZ = getChunkCoord(cameraPos.z);

    if (!initialized || newCenterChunkX != centerChunkX || newCenterChunkZ != centerChunkZ) {
        centerChunkX = newCenterChunkX;
        centerChunkZ = newCenterChunkZ;
        rebuildChunks();
        initialized = true;
    }
}

void WorldManager::rebuildChunks() {
    chunks.clear();

    for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
        for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
            int chunkX = centerChunkX + dx;
            int chunkZ = centerChunkZ + dz;

            auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);
            chunk->initialize();
            chunks.push_back(std::move(chunk));
        }
    }

    std::cout << "Loaded 7x7 grid centered at ("
              << centerChunkX << ", "
              << centerChunkZ << ")\n";
}

void WorldManager::render(const glm::mat4& viewProjectionMatrix) {
    for (auto& chunk : chunks) {
        chunk->render(viewProjectionMatrix);
    }
}
