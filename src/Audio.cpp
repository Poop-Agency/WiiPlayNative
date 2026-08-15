#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

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
    : m_currentBGM(BGMTrack::None)
    , m_bgmTime(0.0f)
    , m_bgmVolume(0.55f)
    , m_initialized(false)
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

        // Initialize 44.1kHz Stereo BGM Stream
        SetAudioStreamBufferSizeDefault(4096);
        m_bgmStream = LoadAudioStream(44100, 16, 2);
        PlayAudioStream(m_bgmStream);

        m_initialized = true;
        std::cout << "Audio device and procedural BGM streamer initialized." << std::endl;
    }
}

void AudioManager::Close() {
    if (!m_initialized) return;

    StopAudioStream(m_bgmStream);
    UnloadAudioStream(m_bgmStream);

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

void AudioManager::PlayBGM(BGMTrack track) {
    m_currentBGM = track;
}

void AudioManager::StopBGM() {
    m_currentBGM = BGMTrack::None;
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

// Procedural real-time Wii Play Tanks theme music generator
void AudioManager::Update(float dt) {
    if (!m_initialized) return;

    if (IsAudioStreamProcessed(m_bgmStream)) {
        const int FRAMES_TO_STREAM = 4096;
        short buffer[FRAMES_TO_STREAM * 2];

        float sampleRate = 44100.0f;
        float dtSample = 1.0f / sampleRate;

        // Wii Tanks Tempo: 118 BPM
        float bpm = 118.0f;
        float beatDuration = 60.0f / bpm;

        // Note frequencies (D minor / F Major)
        float fD2 = 73.42f;
        float fF2 = 87.31f;
        float fG2 = 98.00f;
        float fA2 = 110.0f;
        float fD3 = 146.83f;
        float fF3 = 174.61f;
        float fA3 = 220.0f;
        float fC4 = 261.63f;
        float fD4 = 293.66f;
        float fE4 = 329.63f;
        float fF4 = 349.23f;
        float fA4 = 440.0f;

        for (int i = 0; i < FRAMES_TO_STREAM; ++i) {
            float t = m_bgmTime;
            m_bgmTime += dtSample;

            if (m_currentBGM == BGMTrack::None) {
                buffer[i * 2] = 0;
                buffer[i * 2 + 1] = 0;
                continue;
            }

            float currentBeat = t / beatDuration;
            int beatIndex = static_cast<int>(currentBeat);
            float beatFrac = currentBeat - float(beatIndex);

            int barStep = beatIndex % 16; // 4 bars loop
            int sixteenth = static_cast<int>(beatFrac * 4.0f);
            float sixteenthFrac = (beatFrac * 4.0f) - float(sixteenth);

            float outL = 0.0f;
            float outR = 0.0f;

            if (m_currentBGM == BGMTrack::Gameplay || m_currentBGM == BGMTrack::Title) {
                // 1. Bass Groove (Signature D minor punchy bassline)
                float bassFreq = fD2;
                if (barStep == 2 || barStep == 10) bassFreq = fF2;
                else if (barStep == 3 || barStep == 11) bassFreq = fG2;
                else if (barStep == 6 || barStep == 14) bassFreq = fA2;
                else if (barStep == 7 || barStep == 15) bassFreq = fD3;

                float bassEnv = std::exp(-beatFrac * 6.0f);
                float bassTone = std::sin(t * bassFreq * 2.0f * PI) * 0.45f + 
                                 std::sin(t * bassFreq * 2.0f * 2.0f * PI) * 0.15f;
                float bass = bassTone * bassEnv;

                // 2. Woody Marimba / Arpeggio Chords
                float marimbaFreq = 0.0f;
                int arpStep = (barStep * 4 + sixteenth) % 16;
                const float arpTable[16] = { fD4, fF4, fA4, fD4, fF4, fA4, fC4, fD4, fE4, fF4, fA4, fF4, fE4, fD4, fA3, fD4 };
                marimbaFreq = arpTable[arpStep];

                float marimbaEnv = std::exp(-sixteenthFrac * 12.0f);
                float marimba = (std::sin(t * marimbaFreq * 2.0f * PI) * 0.28f +
                                 std::sin(t * marimbaFreq * 3.0f * 2.0f * PI) * 0.08f) * marimbaEnv;

                // 3. Drums: Kick & Snare & Hi-Hat
                float kick = 0.0f;
                if (barStep % 2 == 0) {
                    float kEnv = std::exp(-beatFrac * 16.0f);
                    kick = std::sin(t * (60.0f - beatFrac * 30.0f) * 2.0f * PI) * 0.35f * kEnv;
                }

                float snare = 0.0f;
                if (barStep % 2 == 1) {
                    float sEnv = std::exp(-beatFrac * 14.0f);
                    float noise = ((rand() % 100) / 50.0f - 1.0f);
                    snare = (noise * 0.25f + std::sin(t * 190.0f * 2.0f * PI) * 0.15f) * sEnv;
                }

                float hihat = 0.0f;
                float hEnv = std::exp(-sixteenthFrac * 25.0f);
                hihat = ((rand() % 100) / 50.0f - 1.0f) * 0.08f * hEnv;

                // Stereo Mix
                outL = (bass * 0.5f + marimba * 0.6f + kick * 0.4f + snare * 0.3f + hihat * 0.3f) * m_bgmVolume;
                outR = (bass * 0.5f + marimba * 0.4f + kick * 0.4f + snare * 0.3f + hihat * 0.5f) * m_bgmVolume;
            }

            outL = std::clamp(outL, -1.0f, 1.0f);
            outR = std::clamp(outR, -1.0f, 1.0f);

            buffer[i * 2] = static_cast<short>(outL * 32767.0f);
            buffer[i * 2 + 1] = static_cast<short>(outR * 32767.0f);
        }

        UpdateAudioStream(m_bgmStream, buffer, FRAMES_TO_STREAM);
    }
}
