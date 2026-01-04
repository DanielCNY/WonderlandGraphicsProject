#include "world_manager.h"
#include "../entities/chunk.h"
#include <cmath>
#include <iostream>

static constexpr int CHUNK_RADIUS = 3;

WorldManager::WorldManager()
    : centerChunkX(0), centerChunkZ(0), initialized(false)
{
    std::cout << "WorldManager constructor" << std::endl;
}

WorldManager::~WorldManager() {
    chunkMap.clear();
}

int WorldManager::getChunkCoord(float worldCoord) {
    return static_cast<int>(std::round(worldCoord / Chunk::SIZE));
}

void WorldManager::update(const glm::vec3& cameraPos) {
    int newCenterChunkX = getChunkCoord(cameraPos.x);
    int newCenterChunkZ = getChunkCoord(cameraPos.z);

    if (!initialized || newCenterChunkX != centerChunkX || newCenterChunkZ != centerChunkZ) {
        centerChunkX = newCenterChunkX;
        centerChunkZ = newCenterChunkZ;

        std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, PairHash> newChunkMap;

        for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
            for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
                int chunkX = centerChunkX + dx;
                int chunkZ = centerChunkZ + dz;
                auto key = std::make_pair(chunkX, chunkZ);
                auto it = chunkMap.find(key);
                if (it != chunkMap.end()) {
                    newChunkMap[key] = std::move(it->second);
                }
                else {
                    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);
                    chunk->initialize();
                    newChunkMap[key] = std::move(chunk);
                }
            }
        }
        chunkMap = std::move(newChunkMap);

        initialized = true;
    }
}

void WorldManager::render(const glm::mat4& viewProjectionMatrix) {
    for (auto& pair : chunkMap) {
        pair.second->render(viewProjectionMatrix);
    }
}