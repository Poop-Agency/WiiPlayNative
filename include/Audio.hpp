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

enum class BGMTrack {
    None,
    Title,
    Gameplay,
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
    void Update(float dt);

    void PlayBGM(BGMTrack track);
    void StopBGM();

private:
    void GenerateProceduralSounds();
    void FillAudioBuffer(void* buffer, unsigned int frames);

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

    AudioStream m_bgmStream;
    BGMTrack m_currentBGM;
    float m_bgmTime;
    float m_bgmVolume;
    bool m_initialized;
};
