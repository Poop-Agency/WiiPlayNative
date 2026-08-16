#include "Audio.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <string>

// Tread click spacing at full speed: 0.373 s cycle / 4 clips, matching the original mix.
static constexpr float kTreadInterval = 0.0932f;

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
    , m_sndTread{}
    , m_hasTread(false)
    , m_treadIdx(0)
    , m_treadTimer(0.0f)
    , m_sndTankHit{}
    , m_sndTankExplode{}
    , m_hasShoot(false)
    , m_hasRocket(false)
    , m_hasShootP2(false)
    , m_hasShootEnemy{false}
    , m_hasTankHit(false)
    , m_hasTankExplode(false)
    , m_hasRicochet(false)
    , m_hasBlockBreak(false)
    , m_hasMinePlant(false)
    , m_hasMineBeep(false)
    , m_hasMineTrigger(false)
    , m_hasExplosion(false)
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
        LoadRippedSounds();
        LoadTreadSounds();
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

// Wave index -> file under assets/sfx/raw, whose names carry the index as a
// two-digit prefix followed by the rate and duration. Resolved by scanning the
// directory so the rest of the filename stays out of the source.
static std::string RawWavePath(int waveIndex) {
    char prefix[8];
    std::snprintf(prefix, sizeof(prefix), "%02d_", waveIndex);
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator("assets/sfx/raw", ec)) {
        std::string n = e.path().filename().string();
        if (n.rfind(prefix, 0) == 0) return e.path().string();
    }
    return {};
}

static bool LoadWave(Sound& dst, int waveIndex, float volume) {
    std::string path = RawWavePath(waveIndex);
    if (path.empty()) return false;
    Sound s = LoadSound(path.c_str());
    if (!IsSoundValid(s)) return false;
    if (IsSoundValid(dst)) UnloadSound(dst);
    dst = s;
    SetSoundVolume(dst, volume);
    return true;
}

// Every index below comes from tools/brsar_map.py, which walks name -> RSEQ
// label -> RBNK program -> wave index. The RP_TNK_SE_*.wav files in assets/sfx
// are mis-named and are deliberately not used here.
//
//   SHOOT_1P / SHOOT_2P        program 0  -> wave 0
//   SHOOT_ENEMY1,2,4,5,6,8     program 0  -> wave 0
//   SHOOT_ENEMY3, ENEMY9       program 21 -> wave 2
//   SHOOT_ENEMY7               program 20 -> wave 3
//   HIT and JIRAI_EXP          program 1  -> wave 6   (one sample for both)
//   REFLECT                    program 2  -> wave 7
//   JIRAI_SET                  program 3  -> wave 9
//   JIRAI_EXP_MAE              program 4  -> wave 10  ("mae" = before, the alert)
//   JIRAI_TIMER                program 5  -> wave 11
//   BROKEN                     program 18 -> wave 4
//   TNK_DISAPPEAR              program 17 -> wave 26
//
// Enemies 3, 7 and 9 are Teal, Green and Black, the three rocket tanks, which
// is why only those three carry a firing sound of their own.
void AudioManager::LoadRippedSounds() {
    m_hasShoot       = LoadWave(m_sndShoot,       0,  0.40f);
    m_hasShootP2     = LoadWave(m_sndShootP2,     0,  0.40f);
    m_hasTankHit     = LoadWave(m_sndTankHit,     6,  0.40f);
    m_hasExplosion   = LoadWave(m_sndExplosion,   6,  0.45f);
    m_hasTankExplode = LoadWave(m_sndTankExplode, 26, 0.40f);
    m_hasRicochet    = LoadWave(m_sndRicochet,    7,  0.35f);
    m_hasBlockBreak  = LoadWave(m_sndBlockBreak,  4,  0.35f);
    m_hasMinePlant   = LoadWave(m_sndMinePlant,   9,  0.35f);
    m_hasMineTrigger = LoadWave(m_sndMineTrigger, 10, 0.35f);
    m_hasMineBeep    = LoadWave(m_sndMineBeep,    11, 0.25f);
    m_hasRocket      = LoadWave(m_sndRocket,      2,  0.40f);

    // TankType order in our enum is Brown, Ash, Teal, Yellow, Red, Green,
    // Purple, White, Black; the retail e_1..e_9 numbering is Brown, Ash, Teal,
    // Red, Yellow, Purple, Green, White, Black. Indexed here by our enum order.
    static const int kEnemyWave[9] = { 0, 0, 2, 0, 0, 3, 0, 0, 2 };
    for (int i = 0; i < 9; ++i) {
        m_hasShootEnemy[i] = LoadWave(m_sndShootEnemy[i], kEnemyWave[i], 0.40f);
    }
}

void AudioManager::LoadTreadSounds() {
    // Tread clatter ripped from rp_Tnk_sound.brsar (RP_TNK_BANK_SE_01 waves 15/14/13/12),
    // played round-robin. Falls back to the procedural drive hum if the files are missing.
    m_hasTread = true;
    for (int i = 0; i < 4; ++i) {
        std::string path = "assets/sfx/tread_" + std::to_string(i) + ".wav";
        m_sndTread[i] = LoadSound(path.c_str());
        if (!IsSoundValid(m_sndTread[i])) {
            m_hasTread = false;
        } else {
            SetSoundVolume(m_sndTread[i], 0.22f);
        }
    }
    m_treadIdx = 0;
    m_treadTimer = 0.0f;
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

    if (IsSoundValid(m_sndShoot)) UnloadSound(m_sndShoot);
    if (IsSoundValid(m_sndShootP2)) UnloadSound(m_sndShootP2);
    for (int i = 0; i < 9; ++i) {
        if (IsSoundValid(m_sndShootEnemy[i])) UnloadSound(m_sndShootEnemy[i]);
    }
    if (IsSoundValid(m_sndRocket)) UnloadSound(m_sndRocket);
    if (IsSoundValid(m_sndTankHit)) UnloadSound(m_sndTankHit);
    if (IsSoundValid(m_sndTankExplode)) UnloadSound(m_sndTankExplode);
    if (IsSoundValid(m_sndRicochet)) UnloadSound(m_sndRicochet);
    if (IsSoundValid(m_sndMinePlant)) UnloadSound(m_sndMinePlant);
    if (IsSoundValid(m_sndMineBeep)) UnloadSound(m_sndMineBeep);
    if (IsSoundValid(m_sndMineTrigger)) UnloadSound(m_sndMineTrigger);
    if (IsSoundValid(m_sndExplosion)) UnloadSound(m_sndExplosion);
    if (IsSoundValid(m_sndBlockBreak)) UnloadSound(m_sndBlockBreak);
    if (IsSoundValid(m_sndEngineIdle)) UnloadSound(m_sndEngineIdle);
    if (IsSoundValid(m_sndEngineDrive)) UnloadSound(m_sndEngineDrive);
    for (Sound& s : m_sndTread) {
        if (IsSoundValid(s)) UnloadSound(s);
    }

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
        case SoundType::ShootNormal:  if (IsSoundValid(m_sndShoot)) PlaySound(m_sndShoot); break;
        case SoundType::ShootP2:      if (IsSoundValid(m_sndShootP2)) PlaySound(m_sndShootP2); break;
        case SoundType::ShootEnemy1:  if (IsSoundValid(m_sndShootEnemy[0])) PlaySound(m_sndShootEnemy[0]); break;
        case SoundType::ShootEnemy2:  if (IsSoundValid(m_sndShootEnemy[1])) PlaySound(m_sndShootEnemy[1]); break;
        case SoundType::ShootEnemy3:  if (IsSoundValid(m_sndShootEnemy[2])) PlaySound(m_sndShootEnemy[2]); break;
        case SoundType::ShootEnemy4:  if (IsSoundValid(m_sndShootEnemy[3])) PlaySound(m_sndShootEnemy[3]); break;
        case SoundType::ShootEnemy5:  if (IsSoundValid(m_sndShootEnemy[4])) PlaySound(m_sndShootEnemy[4]); break;
        case SoundType::ShootEnemy6:  if (IsSoundValid(m_sndShootEnemy[5])) PlaySound(m_sndShootEnemy[5]); break;
        case SoundType::ShootEnemy7:  if (IsSoundValid(m_sndShootEnemy[6])) PlaySound(m_sndShootEnemy[6]); break;
        case SoundType::ShootEnemy8:  if (IsSoundValid(m_sndShootEnemy[7])) PlaySound(m_sndShootEnemy[7]); break;
        case SoundType::ShootEnemy9:  if (IsSoundValid(m_sndShootEnemy[8])) PlaySound(m_sndShootEnemy[8]); break;
        case SoundType::ShootRocket:  if (IsSoundValid(m_sndRocket)) PlaySound(m_sndRocket); break;
        case SoundType::TankHit:      if (IsSoundValid(m_sndTankHit)) PlaySound(m_sndTankHit); break;
        case SoundType::TankExplode:  if (IsSoundValid(m_sndTankExplode)) PlaySound(m_sndTankExplode); break;
        case SoundType::Ricochet:     if (IsSoundValid(m_sndRicochet)) PlaySound(m_sndRicochet); break;
        case SoundType::MinePlant:    if (IsSoundValid(m_sndMinePlant)) PlaySound(m_sndMinePlant); break;
        case SoundType::MineBeep:     if (IsSoundValid(m_sndMineBeep)) PlaySound(m_sndMineBeep); break;
        case SoundType::MineTrigger:  if (IsSoundValid(m_sndMineTrigger)) PlaySound(m_sndMineTrigger); break;
        case SoundType::Explosion:    if (IsSoundValid(m_sndExplosion)) PlaySound(m_sndExplosion); break;
        case SoundType::BlockBreak:   if (IsSoundValid(m_sndBlockBreak)) PlaySound(m_sndBlockBreak); break;
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
        SetSoundVolume(m_sndEngineDrive, m_hasTread ? 0.09f : 0.16f);
        if (!IsSoundPlaying(m_sndEngineDrive)) {
            PlaySound(m_sndEngineDrive);
        }

        SetSoundPitch(m_sndEngineIdle, 1.1f);
        SetSoundVolume(m_sndEngineIdle, 0.05f);

        if (m_hasTread) {
            // 93 ms between clicks at full speed = the 0.373 s 4-clip cycle of the original.
            // ponytail: linear inverse scaling, clamped; swap for a curve if slow tanks sound off.
            float interval = kTreadInterval / std::max(speedRatio, 0.35f);
            m_treadTimer += GetFrameTime();
            if (m_treadTimer >= interval) {
                m_treadTimer = 0.0f;
                PlaySound(m_sndTread[m_treadIdx]);
                m_treadIdx = (m_treadIdx + 1) % 4;
            }
        }
    } else {
        if (IsSoundPlaying(m_sndEngineDrive)) {
            StopSound(m_sndEngineDrive);
        }
        m_treadTimer = kTreadInterval;   // next move clicks immediately
        float ambient = std::min(0.14f, 0.08f + movingEnemiesCount * 0.02f);
        SetSoundPitch(m_sndEngineIdle, 0.90f);
        SetSoundVolume(m_sndEngineIdle, ambient);
    }
}

void AudioManager::StopEngineAudio() {
    if (m_enginePlaying) {
        StopSound(m_sndEngineIdle);
        StopSound(m_sndEngineDrive);
        m_treadTimer = kTreadInterval;
        m_treadIdx = 0;
        m_enginePlaying = false;
    }
}

void AudioManager::Update(float dt) {
    if (!m_initialized) return;

    if (m_activeMusic) {
        UpdateMusicStream(*m_activeMusic);
    }
}
