#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <filesystem>

static Sound GenerateFallbackSound(int sampleRate, float duration, auto generator) {
    int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<short> buffer(totalSamples);

    for (int i = 0; i < totalSamples; ++i) {
        float t = float(i) / float(sampleRate);
        float sample = generator(t, duration);
        sample = std::clamp(sample, -0.6f, 0.6f);
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
    , m_volume(0.48f)
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
        std::string sfxDir = "assets/sfx/";

        auto loadOrFallback = [&](const std::string& path, Sound fallback) -> Sound {
            if (std::filesystem::exists(path)) {
                Sound s = LoadSound(path.c_str());
                if (s.frameCount > 0) {
                    SetSoundVolume(s, 0.42f);
                    return s;
                }
            }
            SetSoundVolume(fallback, 0.42f);
            return fallback;
        };

        // Load normalized, authentic Nintendo Wii Play sound effects
        m_sndShoot = loadOrFallback(sfxDir + "shoot_1p.wav", GenerateFallbackSound(44100, 0.16f, [](float t, float) {
            return (std::sin(t * 220.0f * 2.0f * PI) > 0 ? 0.35f : -0.35f) * std::exp(-t * 20.0f);
        }));

        m_sndRocket = loadOrFallback(sfxDir + "shoot_rocket.wav", GenerateFallbackSound(44100, 0.20f, [](float t, float) {
            return std::sin(t * 440.0f * 2.0f * PI) * std::exp(-t * 15.0f) * 0.35f;
        }));

        m_sndRicochet = loadOrFallback(sfxDir + "reflect.wav", GenerateFallbackSound(44100, 0.12f, [](float t, float) {
            return std::sin(t * 1500.0f * 2.0f * PI) * std::exp(-t * 22.0f) * 0.3f;
        }));

        m_sndMinePlant = loadOrFallback(sfxDir + "mine_plant.wav", GenerateFallbackSound(44100, 0.09f, [](float t, float) {
            return std::sin(t * 300.0f * 2.0f * PI) * std::exp(-t * 25.0f) * 0.35f;
        }));

        m_sndMineBeep = loadOrFallback(sfxDir + "mine_beep.wav", GenerateFallbackSound(44100, 0.05f, [](float t, float) {
            return std::sin(t * 1000.0f * 2.0f * PI) * std::exp(-t * 40.0f) * 0.25f;
        }));

        m_sndExplosion = loadOrFallback(sfxDir + "explosion.wav", GenerateFallbackSound(44100, 0.55f, [](float t, float) {
            return (((rand() % 100) / 50.0f) - 1.0f) * std::exp(-t * 5.0f) * 0.45f;
        }));

        m_sndBlockBreak = loadOrFallback(sfxDir + "broken.wav", GenerateFallbackSound(44100, 0.25f, [](float t, float) {
            return (((rand() % 100) / 50.0f) - 1.0f) * std::exp(-t * 16.0f) * 0.4f;
        }));

        m_sndEngineIdle = loadOrFallback(sfxDir + "engine_idle.wav", GenerateFallbackSound(44100, 1.0f, [](float t, float) {
            return std::sin(t * 65.0f * 2.0f * PI) * 0.2f;
        }));

        m_sndEngineDrive = loadOrFallback(sfxDir + "engine_drive.wav", GenerateFallbackSound(44100, 1.0f, [](float t, float) {
            return std::sin(t * 110.0f * 2.0f * PI) * 0.25f;
        }));

        m_sndMissionStart = loadOrFallback("assets/musique/Wii Play - Tanks - Start [Wii Play OST].mp3", GenerateFallbackSound(44100, 0.4f, [](float t, float) {
            return std::sin(t * 523.0f * 2.0f * PI) * std::exp(-t * 5.0f) * 0.4f;
        }));

        m_sndVictory = loadOrFallback("assets/musique/Wii Play - Tanks - Round End [Wii Play OST].mp3", GenerateFallbackSound(44100, 0.6f, [](float t, float) {
            return std::sin(t * 659.0f * 2.0f * PI) * std::exp(-t * 3.0f) * 0.4f;
        }));

        m_sndGameOver = loadOrFallback("assets/musique/Wii Play - Tanks - Round Failed [Wii Play OST].mp3", GenerateFallbackSound(44100, 0.65f, [](float t, float) {
            return std::sin(t * 311.0f * 2.0f * PI) * std::exp(-t * 4.0f) * 0.4f;
        }));

        LoadMP3Tracks();
        m_initialized = true;
        std::cout << "Audio Device initialized cleanly without saturation." << std::endl;
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
        SetSoundVolume(m_sndEngineIdle, 0.16f);
        m_enginePlaying = true;
    }

    if (!IsSoundPlaying(m_sndEngineIdle)) {
        PlaySound(m_sndEngineIdle);
    }

    if (isMoving) {
        float drivePitch = 0.95f + speedRatio * 0.25f;
        SetSoundPitch(m_sndEngineDrive, drivePitch);
        SetSoundVolume(m_sndEngineDrive, 0.20f);
        if (!IsSoundPlaying(m_sndEngineDrive)) {
            PlaySound(m_sndEngineDrive);
        }

        SetSoundPitch(m_sndEngineIdle, 1.1f);
        SetSoundVolume(m_sndEngineIdle, 0.08f);
    } else {
        if (IsSoundPlaying(m_sndEngineDrive)) {
            StopSound(m_sndEngineDrive);
        }
        float ambient = std::min(0.20f, 0.10f + movingEnemiesCount * 0.04f);
        SetSoundPitch(m_sndEngineIdle, 0.90f);
        SetSoundVolume(m_sndEngineIdle, ambient);
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
