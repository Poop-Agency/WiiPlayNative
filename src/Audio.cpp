#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <filesystem>

// Biquad 2-pole Lowpass Filter for clean, band-limited warm synthesis
struct BiquadLowpass {
    float b0, b1, b2, a1, a2;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

    void Init(float cutoff, float sampleRate, float q = 0.707f) {
        float omega = 2.0f * PI * cutoff / sampleRate;
        float alpha = std::sin(omega) / (2.0f * q);
        float cos_w = std::cos(omega);

        float a0 = 1.0f + alpha;
        b0 = ((1.0f - cos_w) * 0.5f) / a0;
        b1 = (1.0f - cos_w) / a0;
        b2 = ((1.0f - cos_w) * 0.5f) / a0;
        a1 = (-2.0f * cos_w) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float Process(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

static Sound GenerateFilteredSound(int sampleRate, float duration, auto generator) {
    int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<short> buffer(totalSamples);

    for (int i = 0; i < totalSamples; ++i) {
        float t = float(i) / float(sampleRate);
        float sample = generator(t, duration, sampleRate);
        sample = std::clamp(sample, -0.75f, 0.75f);
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
    , m_volume(0.45f)
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
        std::cout << "Audio Device initialized with pristine, band-limited lowpass audio synthesis." << std::endl;
    }
}

void AudioManager::GenerateProceduralSounds() {
    int rate = 44100;

    // 1. Normal Shot: Square wave pitch sweep 320 Hz -> 90 Hz with 1800 Hz lowpass noise
    m_sndShoot = GenerateFilteredSound(rate, 0.16f, [f = BiquadLowpass{}](float t, float, float sr) mutable {
        if (t == 0.0f) f.Init(1800.0f, sr);
        float freq = 320.0f * std::pow(90.0f / 320.0f, t / 0.16f);
        float sq = (std::sin(t * freq * 2.0f * PI) > 0.0f ? 0.35f : -0.35f) * std::exp(-t * 22.0f);
        float rawNoise = ((rand() % 100) / 50.0f - 1.0f) * 0.45f * std::exp(-t * 28.0f);
        float filteredNoise = f.Process(rawNoise);
        return (sq + filteredNoise) * 0.55f;
    });

    // 2. Rocket Shot: Higher pitch sweep 640 Hz -> 200 Hz with 3000 Hz lowpass noise
    m_sndRocket = GenerateFilteredSound(rate, 0.12f, [f = BiquadLowpass{}](float t, float, float sr) mutable {
        if (t == 0.0f) f.Init(3000.0f, sr);
        float freq = 640.0f * std::pow(200.0f / 640.0f, t / 0.12f);
        float sq = (std::sin(t * freq * 2.0f * PI) > 0.0f ? 0.30f : -0.30f) * std::exp(-t * 26.0f);
        float rawNoise = ((rand() % 100) / 50.0f - 1.0f) * 0.35f * std::exp(-t * 32.0f);
        float filteredNoise = f.Process(rawNoise);
        return (sq + filteredNoise) * 0.50f;
    });

    // 3. Ricochet: Pure crystalline ping (1500 Hz Triangle + 2300 Hz Sine)
    m_sndRicochet = GenerateFilteredSound(rate, 0.12f, [](float t, float, float) {
        float p1 = (std::asin(std::sin(t * 1500.0f * 2.0f * PI)) / (PI * 0.5f)) * 0.40f * std::exp(-t * 24.0f);
        float p2 = (t > 0.008f) ? (std::sin((t - 0.008f) * 2300.0f * 2.0f * PI) * 0.25f * std::exp(-(t - 0.008f) * 36.0f)) : 0.0f;
        return (p1 + p2) * 0.45f;
    });

    // 4. Mine Plant: Crisp 300 Hz square click
    m_sndMinePlant = GenerateFilteredSound(rate, 0.09f, [](float t, float, float) {
        float sq = (std::sin(t * 300.0f * 2.0f * PI) > 0.0f ? 0.40f : -0.40f);
        return sq * std::exp(-t * 26.0f) * 0.45f;
    });

    // 5. Mine Beep: Pure 1000 Hz sine ping
    m_sndMineBeep = GenerateFilteredSound(rate, 0.05f, [](float t, float, float) {
        return std::sin(t * 1000.0f * 2.0f * PI) * std::exp(-t * 45.0f) * 0.30f;
    });

    // 6. Explosion: Deep sub-bass boom 160 Hz -> 40 Hz + 800 Hz lowpass rumble
    m_sndExplosion = GenerateFilteredSound(rate, 0.50f, [f = BiquadLowpass{}](float t, float, float sr) mutable {
        if (t == 0.0f) f.Init(800.0f, sr);
        float freq = 160.0f * std::pow(40.0f / 160.0f, t / 0.4f);
        float boom = std::sin(t * freq * 2.0f * PI) * 0.45f * std::exp(-t * 5.5f);
        float rawNoise = ((rand() % 100) / 50.0f - 1.0f) * 0.65f * std::exp(-t * 6.0f);
        float filteredNoise = f.Process(rawNoise);
        return (boom + filteredNoise) * 0.50f;
    });

    // 7. Block Break: Snappy cork break with 1200 Hz lowpass
    m_sndBlockBreak = GenerateFilteredSound(rate, 0.22f, [f = BiquadLowpass{}](float t, float, float sr) mutable {
        if (t == 0.0f) f.Init(1200.0f, sr);
        float rawNoise = ((rand() % 100) / 50.0f - 1.0f) * 0.70f * std::exp(-t * 18.0f);
        float thud = std::sin(t * 140.0f * 2.0f * PI) * 0.35f * std::exp(-t * 22.0f);
        return (f.Process(rawNoise) + thud) * 0.45f;
    });

    // 8. Tank Engine Idle: Warm, soft purr (65 Hz + 130 Hz pure sines)
    m_sndEngineIdle = GenerateFilteredSound(rate, 1.2f, [](float t, float, float) {
        float h1 = std::sin(t * 65.0f * 2.0f * PI) * 0.35f;
        float h2 = std::sin(t * 130.0f * 2.0f * PI) * 0.15f;
        return (h1 + h2) * 0.25f;
    });

    // 9. Tank Engine Drive: Smooth motor (110 Hz) with gentle tread clicks (20 Hz)
    m_sndEngineDrive = GenerateFilteredSound(rate, 1.2f, [f = BiquadLowpass{}](float t, float, float sr) mutable {
        if (t == 0.0f) f.Init(1500.0f, sr);
        float motor = std::sin(t * 110.0f * 2.0f * PI) * 0.25f;
        float click = std::pow(std::sin(t * 20.0f * 2.0f * PI), 8.0f) * 0.30f;
        return (motor + f.Process(click)) * 0.30f;
    });

    // Set gentle baseline volumes
    SetSoundVolume(m_sndShoot, 0.40f);
    SetSoundVolume(m_sndRocket, 0.40f);
    SetSoundVolume(m_sndRicochet, 0.35f);
    SetSoundVolume(m_sndMinePlant, 0.35f);
    SetSoundVolume(m_sndMineBeep, 0.25f);
    SetSoundVolume(m_sndExplosion, 0.45f);
    SetSoundVolume(m_sndBlockBreak, 0.35f);
    SetSoundVolume(m_sndEngineIdle, 0.10f);
    SetSoundVolume(m_sndEngineDrive, 0.16f);
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
        case SoundType::MissionStart: PlayBGM(BGMTrack::MissionIntro); break;
        case SoundType::Victory:      PlayBGM(BGMTrack::VictoryJingle); break;
        case SoundType::GameOver:     PlayBGM(BGMTrack::GameOverJingle); break;
        default: break;
    }
}

void AudioManager::UpdateEngineAudio(bool isMoving, float speedRatio, int movingEnemiesCount) {
    if (!m_initialized) return;

    if (!m_enginePlaying) {
        PlaySound(m_sndEngineIdle);
        m_enginePlaying = true;
    }

    if (!IsSoundPlaying(m_sndEngineIdle)) {
        PlaySound(m_sndEngineIdle);
    }

    if (isMoving) {
        float drivePitch = 0.95f + speedRatio * 0.20f;
        SetSoundPitch(m_sndEngineDrive, drivePitch);
        SetSoundVolume(m_sndEngineDrive, 0.16f);
        if (!IsSoundPlaying(m_sndEngineDrive)) {
            PlaySound(m_sndEngineDrive);
        }

        SetSoundPitch(m_sndEngineIdle, 1.1f);
        SetSoundVolume(m_sndEngineIdle, 0.05f);
    } else {
        if (IsSoundPlaying(m_sndEngineDrive)) {
            StopSound(m_sndEngineDrive);
        }
        float ambient = std::min(0.14f, 0.08f + movingEnemiesCount * 0.02f);
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
