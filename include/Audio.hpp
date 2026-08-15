#pragma once

#include "Common.hpp"

enum class SoundType {
    ShootNormal,
    ShootRocket,
    Ricochet,
    MinePlant,
    MineBeep,
    Explosion,
    BlockBreak,
    TankMove,
    MissionStart,
    Victory,
    GameOver
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void Init();
    void Close();
    void Play(SoundType type);
    void UpdateEngineSound(float speedRatio);

private:
    void GenerateProceduralSounds();

    Sound m_sndShoot;
    Sound m_sndRocket;
    Sound m_sndRicochet;
    Sound m_sndMinePlant;
    Sound m_sndMineBeep;
    Sound m_sndExplosion;
    Sound m_sndBlockBreak;
    Sound m_sndMissionStart;
    Sound m_sndVictory;
    Sound m_sndGameOver;

    bool m_initialized;
};
