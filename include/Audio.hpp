#pragma once

#include "Common.hpp"
#include <string>
#include <vector>
#include <unordered_map>

enum class SoundType {
    ShootNormal,
    ShootP2,
    ShootEnemy1,
    ShootEnemy2,
    ShootEnemy3,
    ShootEnemy4,
    ShootEnemy5,
    ShootEnemy6,
    ShootEnemy7,
    ShootEnemy8,
    ShootEnemy9,
    ShootRocket,
    TankHit,
    TankExplode,
    Ricochet,
    MinePlant,
    MineBeep,
    MineTrigger,
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
    void LoadRippedSounds();
    void LoadTreadSounds();
    void LoadMP3Tracks();

    // Sound effects
    Sound m_sndShoot;
    Sound m_sndShootP2;
    Sound m_sndShootEnemy[9];
    Sound m_sndRocket;
    Sound m_sndTankHit;
    Sound m_sndTankExplode;
    Sound m_sndRicochet;
    Sound m_sndMinePlant;
    Sound m_sndMineBeep;
    Sound m_sndMineTrigger;
    Sound m_sndExplosion;
    Sound m_sndBlockBreak;
    Sound m_sndMissionStart;
    Sound m_sndVictory;
    Sound m_sndGameOver;

    bool m_hasShoot;
    bool m_hasRocket;
    bool m_hasShootP2;
    bool m_hasShootEnemy[9];
    bool m_hasTankHit;
    bool m_hasTankExplode;
    bool m_hasRicochet;
    bool m_hasBlockBreak;
    bool m_hasMinePlant;
    bool m_hasMineBeep;
    bool m_hasMineTrigger;
    bool m_hasExplosion;

    // Engine & Tread sound
    Sound m_sndEngineIdle;
    Sound m_sndEngineDrive;
    bool m_enginePlaying;

    // Ripped tread clatter: 4 clips played round-robin, cadence follows speed
    Sound m_sndTread[4];
    bool m_hasTread;
    int m_treadIdx;
    float m_treadTimer;

    // Raylib Music Streaming
    std::unordered_map<BGMTrack, Music> m_musicTracks;
    BGMTrack m_currentTrack;
    Music* m_activeMusic;
    float m_volume;
    bool m_initialized;
    bool m_hasMp3;
};
