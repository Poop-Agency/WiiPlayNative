#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

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
    : m_initialized(false)
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
        m_initialized = true;
    }
}

void AudioManager::Close() {
    if (!m_initialized) return;

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

void AudioManager::GenerateProceduralSounds() {
    int rate = 44100;

    // 1. Shoot Normal: Pop + punchy transient
    m_sndShoot = GenerateSound(rate, 0.18f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 2.5f);
        float freq = 280.0f - (t / dur) * 180.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.3f;
        return (std::sin(t * freq * 2.0f * PI) * 0.7f + noise) * env;
    });

    // 2. Shoot Rocket: High-pitched whoosh
    m_sndRocket = GenerateSound(rate, 0.28f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 1.8f);
        float freq = 450.0f + (t / dur) * 300.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f) * 0.45f;
        return (std::sin(t * freq * 2.0f * PI) * 0.6f + noise) * env;
    });

    // 3. Ricochet: High metallic ping
    m_sndRicochet = GenerateSound(rate, 0.22f, [](float t, float dur) {
        float env = std::exp(-t * 18.0f);
        float freq = 1200.0f - (t / dur) * 400.0f;
        return std::sin(t * freq * 2.0f * PI) * env * 0.8f;
    });

    // 4. Mine Plant: Click
    m_sndMinePlant = GenerateSound(rate, 0.08f, [](float t, float dur) {
        float env = std::exp(-t * 40.0f);
        return std::sin(t * 800.0f * 2.0f * PI) * env;
    });

    // 5. Mine Beep: High tactical blip
    m_sndMineBeep = GenerateSound(rate, 0.05f, [](float t, float dur) {
        float env = std::sin(t / dur * PI);
        return std::sin(t * 1750.0f * 2.0f * PI) * env * 0.5f;
    });

    // 6. Explosion: Heavy sub-bass boom + rumble
    m_sndExplosion = GenerateSound(rate, 0.65f, [](float t, float dur) {
        float env = std::exp(-t * 4.5f);
        float subFreq = 65.0f - (t / dur) * 40.0f;
        float noise = ((rand() % 100) / 50.0f - 1.0f);
        return (std::sin(t * subFreq * 2.0f * PI) * 0.6f + noise * 0.5f) * env;
    });

    // 7. Block Break: Crumble crunch
    m_sndBlockBreak = GenerateSound(rate, 0.25f, [](float t, float dur) {
        float env = std::exp(-t * 10.0f);
        float noise = ((rand() % 100) / 50.0f - 1.0f);
        float tone = std::sin(t * 180.0f * 2.0f * PI);
        return (noise * 0.7f + tone * 0.3f) * env;
    });

    // 8. Mission Start: Ascending jingle
    m_sndMissionStart = GenerateSound(rate, 0.5f, [](float t, float dur) {
        float note1 = (t < 0.16f) ? 523.25f : ((t < 0.32f) ? 659.25f : 783.99f);
        float env = 0.6f * (1.0f - std::fmod(t, 0.16f) / 0.16f);
        return std::sin(t * note1 * 2.0f * PI) * env;
    });

    // 9. Victory: Fanfare chord
    m_sndVictory = GenerateSound(rate, 0.8f, [](float t, float dur) {
        float env = std::pow(1.0f - (t / dur), 1.2f);
        float s1 = std::sin(t * 523.25f * 2.0f * PI);
        float s2 = std::sin(t * 659.25f * 2.0f * PI);
        float s3 = std::sin(t * 783.99f * 2.0f * PI);
        float s4 = std::sin(t * 1046.50f * 2.0f * PI);
        return (s1 + s2 + s3 + s4) * 0.2f * env;
    });

    // 10. Game Over: Descending minor tones
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

void AudioManager::UpdateEngineSound(float speedRatio) {
}
