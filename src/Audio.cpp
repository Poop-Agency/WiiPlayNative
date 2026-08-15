#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <filesystem>

static Sound GenerateSound(int sampleRate, float duration, auto generator) {
    int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<short> buffer(totalSamples);

    for (int i = 0; i < totalSamples; ++i) {
        float t = float(i) / float(sampleRate);
        float sample = generator(t, duration);
        sample = std::clamp(sample, -1.0f, 1.0f);
        buffer[i] = static_cast<short>(sample * 32767.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = totalSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = buffer.data();

    Sound snd = LoadSoundFromWave(wave);
    return snd;
}

AudioManager::AudioManager()
    : m_currentTrack(BGMTrack::None)
    , m_activeMusic(nullptr)
    , m_volume(0.70f)
    , m_initialized(false)
    , m_hasMp3(false)
    , m_enginePlaying(false)
{
}

AudioManager::~AudioManager() {
    Close();
}

void AudioManager::Init() {
    if (m_initialized) return;

    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        GenerateProceduralSounds();
        LoadMP3Tracks();
        m_initialized = true;
        std::cout << "Audio Device initialized with calibrated DSP sound effects and authentic OST." << std::endl;
    }
}

void AudioManager::LoadMP3Tracks() {
    std::string musicDir = "assets/musique/";

    auto tryLoadMusic = [&](BGMTrack track, const std::string& filename) {
        std::string path = musicDir + filename;
        if (std::filesystem::exists(path)) {
            Music m = LoadMusicStream(path.c_str());
            if (m.stream.buffer != nullptr) {
                m.looping = (track != BGMTrack::MissionIntro && track != BGMTrack::VictoryJingle && track != BGMTrack::GameOverJingle);
                SetMusicVolume(m, m_volume);
                m_musicTracks[track] = m;
                m_hasMp3 = true;
            }
        }
    };

    tryLoadMusic(BGMTrack::Title, "Wii Tanks Master Mod - Music ⧸ OST  ｜  Complete.mp3");
    tryLoadMusic(BGMTrack::MissionIntro, "Wii Play - Tanks - Start [Wii Play OST].mp3");
    tryLoadMusic(BGMTrack::VictoryJingle, "Wii Play - Tanks - Round End [Wii Play OST].mp3");
    tryLoadMusic(BGMTrack::GameOverJingle, "Wii Play - Tanks - Round Failed [Wii Play OST].mp3");

    tryLoadMusic(BGMTrack::BrownTank, "Wii Play Tanks! Music - Brown Tank.mp3");
    tryLoadMusic(BGMTrack::AshTank, "Wii Play Tanks! Music - Grey⧸Ash Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::TealTank, "Wii Play Tanks! Music - Teal⧸Marine Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::YellowTank, "Wii Play Tanks! Music - Yellow Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::RedTank, "Wii Play Tanks! Music - Red⧸Pink Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::GreenTank, "Wii Play Tanks! Music - Green Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::PurpleTank, "Wii Play Tanks! Music - Purple⧸Violet Tank (Variant 1).mp3");
    tryLoadMusic(BGMTrack::WhiteTank, "Wii Play Tanks! Music - White Tank (Variant 1) {REUPLOAD}.mp3");
    tryLoadMusic(BGMTrack::BlackTank, "Wii Play Tanks! Music - Black Tank.mp3");
}

void AudioManager::Close() {
    if (!m_initialized) return;

    StopEngineAudio();
    StopBGM();

    for (auto& pair : m_musicTracks) {
        UnloadMusicStream(pair.second);
    }
    m_musicTracks.clear();

    UnloadSound(m_sndShoot);
    UnloadSound(m_sndRocket);
    UnloadSound(m_sndRicochet);
    UnloadSound(m_sndMinePlant);
    UnloadSound(m_sndMineBeep);
    UnloadSound(m_sndExplosion);
    UnloadSound(m_sndBlockBreak);
    UnloadSound(m_sndMissionStart);
    UnloadSound(m_sndVictory);
    UnloadSound(m_sndGameOver);
    UnloadSound(m_sndEngineIdle);
    UnloadSound(m_sndEngineDrive);

    CloseAudioDevice();
    m_initialized = false;
}

void AudioManager::SetVolume(float vol) {
    m_volume = std::clamp(vol, 0.0f, 1.0f);
    if (m_activeMusic) {
        SetMusicVolume(*m_activeMusic, m_volume);
    }
}

void AudioManager::PlayBGM(BGMTrack track) {
    if (m_currentTrack == track && m_activeMusic && IsMusicStreamPlaying(*m_activeMusic)) {
        return;
    }

    if (m_activeMusic) {
        StopMusicStream(*m_activeMusic);
        m_activeMusic = nullptr;
    }

    m_currentTrack = track;

    auto it = m_musicTracks.find(track);
    if (it != m_musicTracks.end()) {
        m_activeMusic = &it->second;
        SetMusicVolume(*m_activeMusic, m_volume);
        PlayMusicStream(*m_activeMusic);
    }
}

void AudioManager::PlayMissionBGM(TankType dominantEnemy) {
    switch (dominantEnemy) {
        case TankType::EnemyBlack:  PlayBGM(BGMTrack::BlackTank); break;
        case TankType::EnemyWhite:  PlayBGM(BGMTrack::WhiteTank); break;
        case TankType::EnemyPurple: PlayBGM(BGMTrack::PurpleTank); break;
        case TankType::EnemyGreen:  PlayBGM(BGMTrack::GreenTank); break;
        case TankType::EnemyRed:    PlayBGM(BGMTrack::RedTank); break;
        case TankType::EnemyYellow: PlayBGM(BGMTrack::YellowTank); break;
        case TankType::EnemyTeal:   PlayBGM(BGMTrack::TealTank); break;
        case TankType::EnemyAsh:    PlayBGM(BGMTrack::AshTank); break;
        case TankType::EnemyBrown:  PlayBGM(BGMTrack::BrownTank); break;
        default:                    PlayBGM(BGMTrack::BrownTank); break;
    }
}

void AudioManager::StopBGM() {
    if (m_activeMusic) {
        StopMusicStream(*m_activeMusic);
        m_activeMusic = nullptr;
    }
    m_currentTrack = BGMTrack::None;
}

void AudioManager::GenerateProceduralSounds() {
    int rate = 44100;

    // Authentic Wii Tanks Shot: Square sweep 320 Hz -> 90 Hz + Lowpass noise
    m_sndShoot = GenerateSound(rate, 0.16f, [](float t, float dur) {
        float f = 320.0f * std::pow(90.0f / 320.0f, t / 0.16f);
        float sq = (std::sin(t * f * 2.0f * PI) > 0.0f ? 1.0f : -1.0f) * 0.5f;
        float env = (t < 0.005f) ? (t / 0.005f) : std::exp(-(t - 0.005f) * 20.0f);
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.32f * std::exp(-t * 28.0f);
        return (sq * 0.7f + noise) * env;
    });

    // Fast Rocket Shot: Square sweep 640 Hz -> 200 Hz + Crisp noise
    m_sndRocket = GenerateSound(rate, 0.11f, [](float t, float dur) {
        float f = 640.0f * std::pow(200.0f / 640.0f, t / 0.11f);
        float sq = (std::sin(t * f * 2.0f * PI) > 0.0f ? 1.0f : -1.0f) * 0.4f;
        float env = (t < 0.004f) ? (t / 0.004f) : std::exp(-(t - 0.004f) * 30.0f);
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.24f * std::exp(-t * 35.0f);
        return (sq * 0.75f + noise) * env;
    });

    // Ricochet: Dual ping at 1500 Hz and 2300 Hz
    m_sndRicochet = GenerateSound(rate, 0.12f, [](float t, float dur) {
        float p1 = std::sin(t * 1500.0f * 2.0f * PI) * 0.7f * std::exp(-t * 22.0f);
        float p2 = (t > 0.01f) ? (std::sin((t - 0.01f) * 2300.0f * 2.0f * PI) * 0.4f * std::exp(-(t - 0.01f) * 35.0f)) : 0.0f;
        return (p1 + p2);
    });

    // Mine Plant: 300 Hz Square Ping
    m_sndMinePlant = GenerateSound(rate, 0.09f, [](float t, float dur) {
        float sq = (std::sin(t * 300.0f * 2.0f * PI) > 0.0f ? 1.0f : -1.0f) * 0.5f;
        return sq * std::exp(-t * 25.0f);
    });

    // Mine Beep: 1000 Hz Sine Ping
    m_sndMineBeep = GenerateSound(rate, 0.05f, [](float t, float dur) {
        return std::sin(t * 1000.0f * 2.0f * PI) * std::exp(-t * 40.0f) * 0.6f;
    });

    // Explosion: Lowpass Sweep 900 Hz -> 120 Hz + Sub-bass Boom
    m_sndExplosion = GenerateSound(rate, 0.55f, [](float t, float dur) {
        float f = 160.0f * std::pow(40.0f / 160.0f, t / 0.4f);
        float boom = std::sin(t * f * 2.0f * PI) * 0.5f * std::exp(-t * 6.0f);
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.55f * std::exp(-t * 5.0f);
        return (boom + noise);
    });

    // Block Break: Snappy Cork Crumble
    m_sndBlockBreak = GenerateSound(rate, 0.25f, [](float t, float dur) {
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.7f * std::exp(-t * 16.0f);
        float punch = std::sin(t * 140.0f * 2.0f * PI) * 0.4f * std::exp(-t * 20.0f);
        return noise + punch;
    });

    // Mission Start Fanfare (Triad Arpeggio C5 -> E5 -> G5 -> C6)
    m_sndMissionStart = GenerateSound(rate, 0.40f, [](float t, float dur) {
        float note = (t < 0.09f) ? 523.25f : ((t < 0.18f) ? 659.25f : ((t < 0.27f) ? 783.99f : 1046.50f));
        float stepT = std::fmod(t, 0.09f);
        float env = 0.6f * std::exp(-stepT * 12.0f);
        return std::sin(t * note * 2.0f * PI) * env;
    });

    // Victory Fanfare
    m_sndVictory = GenerateSound(rate, 0.60f, [](float t, float dur) {
        float note = (t < 0.09f) ? 523.25f : ((t < 0.18f) ? 659.25f : ((t < 0.27f) ? 783.99f : 1046.50f));
        float env = std::exp(-t * 3.0f);
        return std::sin(t * note * 2.0f * PI) * env * 0.5f;
    });

    // Game Over
    m_sndGameOver = GenerateSound(rate, 0.65f, [](float t, float dur) {
        float note = (t < 0.16f) ? 392.0f : ((t < 0.32f) ? 311.0f : 233.0f);
        float stepT = std::fmod(t, 0.16f);
        float env = std::exp(-stepT * 8.0f) * 0.5f;
        return std::sin(t * note * 2.0f * PI) * env;
    });

    // Tank Engine Idle: Mechanical Toy Motor Hum (65 Hz + 130 Hz)
    m_sndEngineIdle = GenerateSound(rate, 1.2f, [](float t, float dur) {
        float h1 = std::sin(t * 65.0f * 2.0f * PI) * 0.5f;
        float h2 = std::sin(t * 130.0f * 2.0f * PI) * 0.25f;
        float pulse = (1.0f + 0.2f * std::sin(t * 16.0f * 2.0f * PI));
        return (h1 + h2) * pulse * 0.35f;
    });

    // Tank Caterpillar Treads Drive: Mechanical Track Clicking (20 Hz Ratchet)
    m_sndEngineDrive = GenerateSound(rate, 1.2f, [](float t, float dur) {
        float motor = std::sin(t * 110.0f * 2.0f * PI) * 0.3f;
        float click = std::pow(std::sin(t * 22.0f * 2.0f * PI), 8.0f) * 0.5f;
        float rattle = ((rand() % 100) / 50.0f - 1.0f) * 0.15f;
        return (motor + click + rattle) * 0.45f;
    });
}

void AudioManager::Play(SoundType type) {
    if (!m_initialized) return;

    switch (type) {
        case SoundType::ShootNormal:  PlaySound(m_sndShoot); break;
        case SoundType::ShootRocket:  PlaySound(m_sndRocket); break;
        case SoundType::Ricochet:     PlaySound(m_sndRicochet); break;
        case SoundType::MinePlant:    PlaySound(m_sndMinePlant); break;
        case SoundType::MineBeep:     PlaySound(m_sndMineBeep); break;
        case SoundType::Explosion:    PlaySound(m_sndExplosion); break;
        case SoundType::BlockBreak:   PlaySound(m_sndBlockBreak); break;
        case SoundType::MissionStart: PlaySound(m_sndMissionStart); break;
        case SoundType::Victory:      PlaySound(m_sndVictory); break;
        case SoundType::GameOver:     PlaySound(m_sndGameOver); break;
        default: break;
    }
}

void AudioManager::UpdateEngineAudio(bool isMoving, float speedRatio, int movingEnemiesCount) {
    if (!m_initialized) return;

    if (!m_enginePlaying) {
        PlaySound(m_sndEngineIdle);
        PlaySound(m_sndEngineDrive);
        m_enginePlaying = true;
    }

    if (!IsSoundPlaying(m_sndEngineIdle)) PlaySound(m_sndEngineIdle);
    if (!IsSoundPlaying(m_sndEngineDrive)) PlaySound(m_sndEngineDrive);

    if (isMoving) {
        float drivePitch = 0.95f + speedRatio * 0.4f;
        SetSoundPitch(m_sndEngineDrive, drivePitch);
        SetSoundVolume(m_sndEngineDrive, 0.45f);

        SetSoundPitch(m_sndEngineIdle, 1.2f);
        SetSoundVolume(m_sndEngineIdle, 0.15f);
    } else {
        float ambientEnemies = std::min(1.0f, movingEnemiesCount * 0.25f);
        SetSoundPitch(m_sndEngineIdle, 0.85f);
        SetSoundVolume(m_sndEngineIdle, 0.25f);

        SetSoundPitch(m_sndEngineDrive, 0.85f);
        SetSoundVolume(m_sndEngineDrive, 0.15f * ambientEnemies);
    }
}

void AudioManager::StopEngineAudio() {
    if (m_enginePlaying) {
        StopSound(m_sndEngineIdle);
        StopSound(m_sndEngineDrive);
        m_enginePlaying = false;
    }
}

void AudioManager::Update(float dt) {
    if (!m_initialized) return;

    if (m_activeMusic) {
        UpdateMusicStream(*m_activeMusic);
    }
}
