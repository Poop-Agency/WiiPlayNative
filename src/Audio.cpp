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
        std::cout << "Audio Device initialized with " << m_musicTracks.size() << " authentic MP3 music tracks." << std::endl;
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

    m_sndShoot = GenerateSound(rate, 0.18f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 2.5f);
        float freq = 280.0f - (t / dur) * 180.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.3f;
        return (std::sin(t * freq * 2.0f * PI) * 0.7f + noise) * env;
    });

    m_sndRocket = GenerateSound(rate, 0.28f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 1.8f);
        float freq = 450.0f + (t / dur) * 300.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.45f;
        return (std::sin(t * freq * 2.0f * PI) * 0.6f + noise) * env;
    });

    m_sndRicochet = GenerateSound(rate, 0.22f, [](float t, float dur) {
        float env = std::exp(-t * 18.0f);
        float freq = 1200.0f - (t / dur) * 400.0f;
        return std::sin(t * freq * 2.0f * PI) * env * 0.8f;
    });

    m_sndMinePlant = GenerateSound(rate, 0.08f, [](float t, float dur) {
        float env = std::exp(-t * 40.0f);
        return std::sin(t * 800.0f * 2.0f * PI) * env;
    });

    m_sndMineBeep = GenerateSound(rate, 0.05f, [](float t, float dur) {
        float env = std::sin(t / dur * PI);
        return std::sin(t * 1750.0f * 2.0f * PI) * env * 0.5f;
    });

    m_sndExplosion = GenerateSound(rate, 0.65f, [](float t, float dur) {
        float env = std::exp(-t * 4.5f);
        float subFreq = 65.0f - (t / dur) * 40.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f);
        return (std::sin(t * subFreq * 2.0f * PI) * 0.6f + noise * 0.5f) * env;
    });

    m_sndBlockBreak = GenerateSound(rate, 0.25f, [](float t, float dur) {
        float env = std::exp(-t * 10.0f);
        float noise = ((rand() % 100) / 50.0f - 1.0f);
        float tone = std::sin(t * 180.0f * 2.0f * PI);
        return (noise * 0.7f + tone * 0.3f) * env;
    });

    m_sndMissionStart = GenerateSound(rate, 0.45f, [](float t, float dur) {
        float note = (t < 0.15f) ? 523.25f : ((t < 0.30f) ? 659.25f : 783.99f);
        float env = 0.6f * (1.0f - std::fmod(t, 0.15f) / 0.15f);
        return std::sin(t * note * 2.0f * PI) * env;
    });

    m_sndVictory = GenerateSound(rate, 0.8f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 1.2f);
        float s1 = std::sin(t * 523.25f * 2.0f * PI);
        float s2 = std::sin(t * 659.25f * 2.0f * PI);
        float s3 = std::sin(t * 783.99f * 2.0f * PI);
        float s4 = std::sin(t * 1046.50f * 2.0f * PI);
        return (s1 + s2 + s3 + s4) * 0.2f * env;
    });

    m_sndGameOver = GenerateSound(rate, 0.7f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 1.5f);
        float note = 392.0f - (t / dur) * 160.0f;
        return std::sin(t * note * 2.0f * PI) * env * 0.5f;
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

void AudioManager::Update(float dt) {
    if (!m_initialized) return;

    if (m_activeMusic) {
        UpdateMusicStream(*m_activeMusic);
    }
}
