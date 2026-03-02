/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * SoundEffect.cpp
 * 替代原 DirectX::Sound 类。
 */

#include "SoundEffect.h"

using namespace cocos2d;
using namespace cocos2d::experimental;

namespace TopSpeed
{

SoundEffect::SoundEffect(const std::string& filename, bool enable3d, int nBuffers)
    : m_filename(filename)
    , m_enable3d(enable3d)
    , m_nBuffers(nBuffers > 0 ? nBuffers : 1)
    , m_volume(100)
    , m_pan(0)
    , m_pitch(1.0f)
    , m_position3d(Vec3::ZERO)
{
    CCLOG("(+) SoundEffect : %s, 3d=%d, nBuffers=%d", filename.c_str(), (int)enable3d, m_nBuffers);

    // 对应原 Sound 构造时预分配多个 IDirectSoundBuffer
    // 此处初始化 nBuffers 个槽，全部置为 INVALID_AUDIO_ID（空闲）
    m_slots.assign(m_nBuffers, AudioEngine::INVALID_AUDIO_ID);

    // 对应原 Sound 构造时预加载音频数据
    // AudioEngine::preload 将音频文件缓存到内存，避免首次播放时的 I/O 延迟
    AudioEngine::preload(filename);
    CCLOG("SoundEffect : preloaded %s", filename.c_str());
}

SoundEffect::~SoundEffect()
{
    CCLOG("(-) SoundEffect : %s", m_filename.c_str());
    stop();
    AudioEngine::uncache(m_filename);
}

int SoundEffect::play(bool looped)
{
    // 对应原 Sound::play(priority, looped)
    int slotIndex = getFreeSlot();
    if (slotIndex < 0)
    {
        CCLOG("SoundEffect::play : no free slot for %s (all %d slots busy)", m_filename.c_str(), m_nBuffers);
        return -1;
    }

    // 在空闲槽中播放音效
    float vol = m_volume / 100.0f;
    int audioId = AudioEngine::play2d(m_filename, looped, vol);

    if (audioId == AudioEngine::INVALID_AUDIO_ID)
    {
        CCLOGERROR("(!) SoundEffect::play : AudioEngine::play2d failed for %s", m_filename.c_str());
        return -1;
    }

    m_slots[slotIndex] = audioId;

    // 对应原 DirectSound 缓冲区：播放完毕自动标记为空闲
    // 通过 setFinishCallback 在播放结束时将槽重置为 INVALID_AUDIO_ID
    //
    // 线程安全说明：Cocos2d-x AudioEngine 的 finishCallback 在主线程（Director 主循环）
    // 中分发，与游戏逻辑运行于同一线程，因此访问 m_slots 是安全的。
    const int capturedSlot = slotIndex;
    AudioEngine::setFinishCallback(audioId, [this, capturedSlot](int id, const std::string& filePath)
    {
        // 播放完毕，清除槽（对应原 getFreeBuffer() 中的 DSBSTATUS_PLAYING 检查）
        if (capturedSlot >= 0 && capturedSlot < (int)m_slots.size() &&
            m_slots[capturedSlot] == id)
        {
            m_slots[capturedSlot] = AudioEngine::INVALID_AUDIO_ID;
        }
    });

    // TODO: 如果 m_enable3d，通过 OpenAL API 设置声源位置：
    //   alSource3f(openalSourceId, AL_POSITION, m_position3d.x, m_position3d.y, m_position3d.z);
    //   alSource3f(openalSourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    //   alSourcei(openalSourceId, AL_SOURCE_RELATIVE, AL_FALSE);
    // 其中 openalSourceId 需从 AudioEngine 内部实现层获取（平台相关）。

    CCLOG("SoundEffect::play : playing %s in slot %d (audioId=%d)", m_filename.c_str(), slotIndex, audioId);
    return slotIndex;
}

void SoundEffect::stop()
{
    // 对应原 Sound::stop()
    for (int i = 0; i < (int)m_slots.size(); ++i)
    {
        if (m_slots[i] != AudioEngine::INVALID_AUDIO_ID)
        {
            AudioEngine::stop(m_slots[i]);
            m_slots[i] = AudioEngine::INVALID_AUDIO_ID;
        }
    }
    CCLOG("SoundEffect::stop : stopped all slots for %s", m_filename.c_str());
}

bool SoundEffect::isPlaying() const
{
    // 对应原 Sound::playing()
    for (int i = 0; i < (int)m_slots.size(); ++i)
    {
        if (m_slots[i] != AudioEngine::INVALID_AUDIO_ID)
        {
            AudioEngine::AudioState state = AudioEngine::getState(m_slots[i]);
            if (state == AudioEngine::AudioState::PLAYING)
            {
                return true;
            }
        }
    }
    return false;
}

void SoundEffect::setVolume(int value)
{
    // 对应原 Sound::volume(Int)，原范围 0~100
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    m_volume = value;

    float vol = value / 100.0f;
    // 更新所有正在播放的槽的音量
    for (int i = 0; i < (int)m_slots.size(); ++i)
    {
        if (m_slots[i] != AudioEngine::INVALID_AUDIO_ID)
        {
            AudioEngine::setVolume(m_slots[i], vol);
        }
    }
}

int SoundEffect::getVolume() const
{
    return m_volume;
}

void SoundEffect::setPan(int value)
{
    // 对应原 Sound::pan(Int)，原范围 -100（左）~ +100（右）
    if (value < -100) value = -100;
    if (value > 100)  value = 100;
    m_pan = value;

    // TODO: Cocos2d-x AudioEngine 不直接支持声像控制。
    //       需要通过 OpenAL API 实现（需 #include <AL/al.h>）：
    //
    //   float panX = value / 100.0f;  // -1.0f（左）~ +1.0f（右）
    //   // 如果 AudioManager::getReverseStereo() == true，则 panX = -panX
    //   for each playing slot:
    //       alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
    //       alSource3f(sourceId, AL_POSITION, panX, 0.0f, 0.0f);
    //
    // 对视障玩家：声像可用于区分赛道左右侧的音效（如左右护栏碰撞）。

    CCLOG("SoundEffect::setPan : pan=%d (OpenAL extension required)", value);
}

void SoundEffect::setPitch(float pitch)
{
    // 对应原 Sound::frequency(Int)（原为 Hz 频率，此处改为音调倍率）
    // 1.0f = 原始频率，2.0f = 高八度，0.5f = 低八度
    if (pitch <= 0.0f) pitch = 0.01f;
    m_pitch = pitch;

    // TODO: Cocos2d-x AudioEngine 不直接支持 pitch 控制。
    //       需要通过 OpenAL API 实现（需 #include <AL/al.h>）：
    //
    //   for each playing slot:
    //       alSourcef(sourceId, AL_PITCH, pitch);
    //
    // 对视障玩家：音调变化可用于表示速度变化（引擎声音随车速升降）。

    CCLOG("SoundEffect::setPitch : pitch=%.2f (OpenAL extension required)", pitch);
}

void SoundEffect::setPosition3D(const Vec3& pos)
{
    // 对应原 Sound::position(Vector3)
    m_position3d = pos;

    // TODO: 通过 OpenAL API 更新所有正在播放的槽的声源位置（需 #include <AL/al.h>）：
    //
    //   for each playing slot:
    //       alSource3f(sourceId, AL_POSITION, pos.x, pos.y, pos.z);
    //       alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);  // 世界坐标
    //
    // 完整 HRTF 空间音效实现步骤：
    //   1. 确保 alsoft.ini 已配置 hrtf = true
    //   2. 调用 AudioManager::setListenerPosition() 设置监听器位置
    //   3. 调用 AudioManager::setListenerOrientation() 设置监听器朝向
    //   4. 此处设置各声源的世界坐标位置
    //   OpenAL Soft 将自动计算 HRTF 滤波，玩家通过耳机即可感知声源方向。
    //
    // 对视障玩家：3D 位置是感知赛道上其他赛车方位的关键手段。

    CCLOG("SoundEffect::setPosition3D : (%.2f, %.2f, %.2f) for %s",
          pos.x, pos.y, pos.z, m_filename.c_str());
}

void SoundEffect::reset()
{
    // 对应原 Sound::reset()
    stop();
    m_volume = 100;
    m_pan    = 0;
    m_pitch  = 1.0f;
    m_position3d = Vec3::ZERO;
    CCLOG("SoundEffect::reset : %s", m_filename.c_str());
}

const std::string& SoundEffect::getFilename() const
{
    return m_filename;
}

int SoundEffect::getFreeSlot() const
{
    // 对应原 Sound::getFreeBuffer()
    // 遍历所有槽，找到第一个空闲的：
    //   - INVALID_AUDIO_ID：槽未被使用（空闲）
    //   - 其他值：检查 AudioEngine 状态，非 PLAYING 则视为空闲
    for (int i = 0; i < (int)m_slots.size(); ++i)
    {
        if (m_slots[i] == AudioEngine::INVALID_AUDIO_ID)
        {
            // 槽未被使用
            return i;
        }

        AudioEngine::AudioState state = AudioEngine::getState(m_slots[i]);
        if (state != AudioEngine::AudioState::PLAYING)
        {
            // 槽已播放完毕或处于其他非播放状态，可复用
            return i;
        }
    }
    // 所有槽均被占用
    return -1;
}

} // namespace TopSpeed
