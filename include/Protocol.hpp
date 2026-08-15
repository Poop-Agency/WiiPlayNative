#pragma once

#include <cstdint>

#pragma pack(push, 1)

enum class PacketType : uint8_t {
    ConnectRequest,
    ConnectAccept,
    LevelStart,
    PlayerInput,
    TankStateSync,
    SpawnBullet,
    PlantMine,
    DestroyBlock,
    ExplosionEvent,
    ChatMessage,
    Ping
};

struct PktHeader {
    PacketType type;
    uint32_t sequence;
};

struct PktConnectRequest {
    PktHeader header;
    char playerName[32];
};

struct PktConnectAccept {
    PktHeader header;
    uint8_t playerId;
    uint8_t currentMission;
    uint8_t totalPlayers;
};

struct PktLevelStart {
    PktHeader header;
    uint8_t missionNumber;
    uint8_t is2Player;
};

struct PktPlayerInput {
    PktHeader header;
    uint8_t playerId;
    float moveX;
    float moveY;
    float aimX;
    float aimY;
    uint8_t shoot;
    uint8_t mine;
};

struct PktTankStateSync {
    PktHeader header;
    uint8_t tankId;
    float posX;
    float posY;
    float chassisAngle;
    float turretAngle;
    uint8_t isAlive;
    uint8_t lives;
};

struct PktSpawnBullet {
    PktHeader header;
    uint32_t bulletId;
    uint8_t ownerId;
    float posX;
    float posY;
    float dirX;
    float dirY;
    float speed;
    uint8_t bounces;
    uint8_t isRocket;
};

struct PktPlantMine {
    PktHeader header;
    uint32_t mineId;
    uint8_t ownerId;
    float posX;
    float posY;
};

struct PktDestroyBlock {
    PktHeader header;
    uint8_t gridX;
    uint8_t gridY;
};

struct PktExplosionEvent {
    PktHeader header;
    float posX;
    float posY;
    float radius;
};

struct PktChatMessage {
    PktHeader header;
    uint8_t senderId;
    char message[64];
};

#pragma pack(pop)
