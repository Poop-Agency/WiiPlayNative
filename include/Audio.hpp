#pragma once

#include "Common.hpp"
#include <string>
#include <vector>
#include <unordered_map>

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
    MissionIntro,
    BrownTank,
    AshTank,
    TealTank,
    YellowTank,
    RedTank,
    GreenTank,
    PurpleTank,
    WhiteTank,
    BlackTank,
    VictoryJingle,
    GameOverJingle
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
    void PlayMissionBGM(TankType dominantEnemy);
    void StopBGM();
    void SetVolume(float vol);

    // Dynamic Engine & Tread Audio
    void UpdateEngineAudio(bool isMoving, float speedRatio, int movingEnemiesCount);
    void StopEngineAudio();

private:
    void GenerateProceduralSounds();
    void LoadMP3Tracks();

    // Sound effects
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

    // Engine & Tread sound
    Sound m_sndEngineIdle;
    Sound m_sndEngineDrive;
    bool m_enginePlaying;

    // Raylib Music Streaming
    std::unordered_map<BGMTrack, Music> m_musicTracks;
    BGMTrack m_currentTrack;
    Music* m_activeMusic;
    float m_volume;
    bool m_initialized;
    bool m_hasMp3;
};
