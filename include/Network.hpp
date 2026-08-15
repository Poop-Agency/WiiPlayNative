#pragma once

#include "Common.hpp"
#include "Protocol.hpp"
#include <enet/enet.h>
#include <string>
#include <vector>
#include <functional>

enum class NetworkRole {
    Offline,
    Server,
    Client
};

struct RemotePlayer {
    ENetPeer* peer;
    uint32_t id;
    std::string name;
    float ping;
};

class GameState;

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool Init();
    void Shutdown();

    bool StartServer(uint16_t port = 7777);
    bool ConnectToServer(const std::string& address, uint16_t port = 7777);
    void Disconnect();

    void Poll(GameState& gameState);

    void SendReliable(const void* data, size_t size);
    void SendUnreliable(const void* data, size_t size);
    void BroadcastReliable(const void* data, size_t size);
    void BroadcastUnreliable(const void* data, size_t size);

    NetworkRole GetRole() const { return m_role; }
    bool IsConnected() const { return m_isConnected; }
    uint32_t GetLocalPlayerId() const { return m_localPlayerId; }
    int GetConnectedPlayerCount() const { return static_cast<int>(m_connectedPeers.size()) + 1; }

private:
    void HandleServerPacket(ENetEvent& event, GameState& gameState);
    void HandleClientPacket(ENetEvent& event, GameState& gameState);

    NetworkRole m_role;
    ENetHost* m_host;
    ENetPeer* m_serverPeer;
    bool m_isConnected;
    uint32_t m_localPlayerId;
    uint32_t m_sequence;
    std::vector<RemotePlayer> m_connectedPeers;
};
