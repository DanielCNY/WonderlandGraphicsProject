#ifndef WORLD_MANAGER_H
#define WORLD_MANAGER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Chunk;

class WorldManager {
public:
    WorldManager();
    ~WorldManager();

    void update(const glm::vec3& cameraPos);
    void render(const glm::mat4& viewProjectionMatrix);

private:
    std::vector<std::unique_ptr<Chunk>> chunks;
    int lastChunkX;
    int lastChunkZ;
    int centerChunkX;
    int centerChunkZ;
    bool initialized;

    int getChunkCoord(float worldCoord);
    void rebuildChunks();
};

#endif