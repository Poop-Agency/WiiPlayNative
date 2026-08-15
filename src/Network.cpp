#include "Network.hpp"
#include "GameState.hpp"
#include <iostream>
#include <cstring>

NetworkManager::NetworkManager()
    : m_role(NetworkRole::Offline)
    , m_host(nullptr)
    , m_serverPeer(nullptr)
    , m_isConnected(false)
    , m_localPlayerId(0)
    , m_sequence(0)
{
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Init() {
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        return false;
    }
    return true;
}

void NetworkManager::Shutdown() {
    Disconnect();
    enet_deinitialize();
}

bool NetworkManager::StartServer(uint16_t port) {
    Disconnect();

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, 8, 2, 0, 0);
    if (!m_host) {
        std::cerr << "Failed to create ENet server host on port " << port << std::endl;
        return false;
    }

    m_role = NetworkRole::Server;
    m_isConnected = true;
    m_localPlayerId = 0; // Host is Player 1
    std::cout << "Server started on port " << port << std::endl;
    return true;
}

bool NetworkManager::ConnectToServer(const std::string& addressStr, uint16_t port) {
    Disconnect();

    m_host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!m_host) {
        std::cerr << "Failed to create ENet client host." << std::endl;
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, addressStr.c_str());
    address.port = port;

    m_serverPeer = enet_host_connect(m_host, &address, 2, 0);
    if (!m_serverPeer) {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        return false;
    }

    m_role = NetworkRole::Client;
    std::cout << "Connecting to " << addressStr << ":" << port << "..." << std::endl;
    return true;
}

void NetworkManager::Disconnect() {
    if (m_serverPeer) {
        enet_peer_disconnect(m_serverPeer, 0);
        m_serverPeer = nullptr;
    }

    for (auto& rp : m_connectedPeers) {
        if (rp.peer) {
            enet_peer_disconnect(rp.peer, 0);
        }
    }
    m_connectedPeers.clear();

    if (m_host) {
        enet_host_flush(m_host);
        enet_host_destroy(m_host);
        m_host = nullptr;
    }

    m_role = NetworkRole::Offline;
    m_isConnected = false;
}

void NetworkManager::SendReliable(const void* data, size_t size) {
    if (m_serverPeer) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(m_serverPeer, 0, packet);
    }
}

void NetworkManager::SendUnreliable(const void* data, size_t size) {
    if (m_serverPeer) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(m_serverPeer, 1, packet);
    }
}

void NetworkManager::BroadcastReliable(const void* data, size_t size) {
    if (m_host) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(m_host, 0, packet);
    }
}

void NetworkManager::BroadcastUnreliable(const void* data, size_t size) {
    if (m_host) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_UNSEQUENCED);
        enet_host_broadcast(m_host, 1, packet);
    }
}

void NetworkManager::Poll(GameState& gameState) {
    if (!m_host) return;

    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (m_role == NetworkRole::Server) {
                    uint32_t assignedId = static_cast<uint32_t>(m_connectedPeers.size()) + 1;
                    RemotePlayer rp;
                    rp.peer = event.peer;
                    rp.id = assignedId;
                    rp.name = "Player " + std::to_string(assignedId + 1);
                    event.peer->data = reinterpret_cast<void*>(uintptr_t(assignedId));
                    m_connectedPeers.push_back(rp);

                    std::cout << "Client connected from " << event.peer->address.host 
                              << ":" << event.peer->address.port << " as ID " << assignedId << std::endl;

                    // Send Accept packet
                    PktConnectAccept accept;
                    accept.header.type = PacketType::ConnectAccept;
                    accept.header.sequence = m_sequence++;
                    accept.playerId = static_cast<uint8_t>(assignedId);
                    accept.currentMission = static_cast<uint8_t>(gameState.GetCurrentMission());
                    accept.totalPlayers = static_cast<uint8_t>(m_connectedPeers.size() + 1);

                    ENetPacket* pkt = enet_packet_create(&accept, sizeof(accept), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, pkt);
                } else if (m_role == NetworkRole::Client) {
                    m_isConnected = true;
                    std::cout << "Connected to server successfully!" << std::endl;

                    // Send connect request
                    PktConnectRequest req;
                    req.header.type = PacketType::ConnectRequest;
                    req.header.sequence = m_sequence++;
                    std::strncpy(req.playerName, "Remote Player", sizeof(req.playerName));

                    ENetPacket* pkt = enet_packet_create(&req, sizeof(req), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(m_serverPeer, 0, pkt);
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (m_role == NetworkRole::Server) {
                    HandleServerPacket(event, gameState);
                } else {
                    HandleClientPacket(event, gameState);
                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Peer disconnected." << std::endl;
                if (m_role == NetworkRole::Client) {
                    m_isConnected = false;
                }
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

void NetworkManager::HandleServerPacket(ENetEvent& event, GameState& gameState) {
    if (event.packet->dataLength < sizeof(PktHeader)) return;
    const PktHeader* header = reinterpret_cast<const PktHeader*>(event.packet->data);

    if (header->type == PacketType::PlayerInput) {
        if (event.packet->dataLength >= sizeof(PktPlayerInput)) {
            const PktPlayerInput* input = reinterpret_cast<const PktPlayerInput*>(event.packet->data);
            for (auto& tank : gameState.GetTanks()) {
                if (tank.GetId() == input->playerId) {
                    tank.moveInput = { input->moveX, input->moveY };
                    tank.aimTarget = { input->aimX, input->aimY };
                    if (input->shoot) tank.shootRequested = true;
                    if (input->mine) tank.mineRequested = true;
                    break;
                }
            }
        }
    }
}

void NetworkManager::HandleClientPacket(ENetEvent& event, GameState& gameState) {
    if (event.packet->dataLength < sizeof(PktHeader)) return;
    const PktHeader* header = reinterpret_cast<const PktHeader*>(event.packet->data);

    if (header->type == PacketType::ConnectAccept) {
        if (event.packet->dataLength >= sizeof(PktConnectAccept)) {
            const PktConnectAccept* accept = reinterpret_cast<const PktConnectAccept*>(event.packet->data);
            m_localPlayerId = accept->playerId;
            std::cout << "Assigned Local Player ID: " << m_localPlayerId << std::endl;
            gameState.StartMission(accept->currentMission, true);
        }
    } else if (header->type == PacketType::TankStateSync) {
        if (event.packet->dataLength >= sizeof(PktTankStateSync)) {
            const PktTankStateSync* sync = reinterpret_cast<const PktTankStateSync*>(event.packet->data);
            for (auto& tank : gameState.GetTanks()) {
                if (tank.GetId() == sync->tankId && sync->tankId != m_localPlayerId) {
                    tank.SetPosition({ sync->posX, sync->posY });
                    tank.SetChassisAngle(sync->chassisAngle);
                    tank.SetTurretAngle(sync->turretAngle);
                    tank.SetAlive(sync->isAlive != 0);
                    tank.SetLives(sync->lives);
                    break;
                }
            }
        }
    } else if (header->type == PacketType::LevelStart) {
        if (event.packet->dataLength >= sizeof(PktLevelStart)) {
            const PktLevelStart* lvl = reinterpret_cast<const PktLevelStart*>(event.packet->data);
            gameState.StartMission(lvl->missionNumber, lvl->is2Player != 0);
        }
    }
}
