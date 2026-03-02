/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * MusicTrack.cpp
 * 替代原 DirectX::MusicSegment 类。
 */

#include "MusicTrack.h"

using namespace cocos2d;
using namespace cocos2d::experimental;

namespace TopSpeed
{

MusicTrack::MusicTrack(const std::string& filename)
    : m_filename(filename)
    , m_audioId(AudioEngine::INVALID_AUDIO_ID)
    , m_volume(100)
    , m_repeats(-1)
    , m_playCount(0)
    , m_preloaded(false)
{
    CCLOG("(+) MusicTrack : %s", filename.c_str());
}

MusicTrack::~MusicTrack()
{
    CCLOG("(-) MusicTrack : %s", m_filename.c_str());
    stop();
    if (m_preloaded)
    {
        AudioEngine::uncache(m_filename);
    }
}

void MusicTrack::preload()
{
    // 对应原 MusicSegment::download()
    // 将音频文件预先加载到内存，避免 play() 时的 I/O 延迟
    if (!m_preloaded)
    {
        AudioEngine::preload(m_filename);
        m_preloaded = true;
        CCLOG("MusicTrack::preload : preloaded %s", m_filename.c_str());
    }
}

void MusicTrack::play(bool looped)
{
    // 对应原 MusicSegment::play(looped, secondary, audioPath)

    // 如果已有实例在播放，先停止
    if (m_audioId != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(m_audioId);
        m_audioId = AudioEngine::INVALID_AUDIO_ID;
    }

    m_playCount = 0;

    if (m_repeats == -1)
    {
        // 无限循环：直接使用 AudioEngine 的 looped 参数
        float vol = m_volume / 100.0f;
        m_audioId = AudioEngine::play2d(m_filename, true, vol);
        if (m_audioId == AudioEngine::INVALID_AUDIO_ID)
        {
            CCLOGERROR("(!) MusicTrack::play : AudioEngine::play2d failed for %s", m_filename.c_str());
            return;
        }
        CCLOG("MusicTrack::play : playing %s (infinite loop, audioId=%d)", m_filename.c_str(), m_audioId);
    }
    else
    {
        // 有限次数或播放一次：使用 finishCallback 计数
        // 对应原 MusicSegment::repeats(times)
        // Cocos2d-x AudioEngine 仅支持 looped=true/false，
        // 有限次重复通过 finishCallback 中自行计数实现。
        (void)looped;
        playInternal();
    }
}

void MusicTrack::playInternal()
{
    float vol = m_volume / 100.0f;
    // 单次播放（不循环），完成后在回调中决定是否再次播放
    m_audioId = AudioEngine::play2d(m_filename, false, vol);
    if (m_audioId == AudioEngine::INVALID_AUDIO_ID)
    {
        CCLOGERROR("(!) MusicTrack::playInternal : AudioEngine::play2d failed for %s", m_filename.c_str());
        return;
    }

    m_playCount++;
    CCLOG("MusicTrack::playInternal : playing %s (count=%d/%d, audioId=%d)",
          m_filename.c_str(), m_playCount, m_repeats, m_audioId);

    // 播放完毕回调：判断是否需要继续播放
    // 线程安全说明：Cocos2d-x AudioEngine 的 finishCallback 在主线程中分发，
    // 与游戏逻辑运行于同一线程，因此访问 m_audioId 和 m_playCount 是安全的。
    AudioEngine::setFinishCallback(m_audioId, [this](int id, const std::string& filePath)
    {
        m_audioId = AudioEngine::INVALID_AUDIO_ID;

        // m_repeats == 0：只播放一次，完毕后不再播放
        // m_repeats > 0：播放指定次数
        if (m_repeats > 0 && m_playCount < m_repeats)
        {
            // 未达到指定次数，继续播放
            playInternal();
        }
        else
        {
            CCLOG("MusicTrack : %s finished (played %d times)", m_filename.c_str(), m_playCount);
        }
    });
}

void MusicTrack::stop(bool immediately)
{
    // 对应原 MusicSegment::stop(immediately)
    //
    // 注意：原 DirectMusic 支持 DMUS_SEGF_MEASURE（等到小节结束后停止），
    //       Cocos2d-x AudioEngine 暂不支持此功能，始终立即停止。
    //       immediately 参数保留以便将来扩展，当前忽略（始终立即停止）。
    (void)immediately;

    if (m_audioId != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(m_audioId);
        m_audioId = AudioEngine::INVALID_AUDIO_ID;
        CCLOG("MusicTrack::stop : stopped %s", m_filename.c_str());
    }
}

bool MusicTrack::isPlaying() const
{
    // 对应原 MusicSegment::isPlaying()
    if (m_audioId == AudioEngine::INVALID_AUDIO_ID)
    {
        return false;
    }
    AudioEngine::AudioState state = AudioEngine::getState(m_audioId);
    return (state == AudioEngine::AudioState::PLAYING);
}

void MusicTrack::setVolume(int value)
{
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    m_volume = value;

    if (m_audioId != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::setVolume(m_audioId, value / 100.0f);
    }
}

int MusicTrack::getVolume() const
{
    return m_volume;
}

void MusicTrack::setRepeats(int times)
{
    // 对应原 MusicSegment::repeats(Int times)
    // -1 = DMUS_SEG_REPEAT_INFINITE（无限循环）
    //  0 = 播放一次
    // >0 = 播放 times 次
    m_repeats = times;
    CCLOG("MusicTrack::setRepeats : %s repeats=%d", m_filename.c_str(), times);
}

int MusicTrack::getRepeats() const
{
    return m_repeats;
}

const std::string& MusicTrack::getFilename() const
{
    return m_filename;
}

} // namespace TopSpeed
