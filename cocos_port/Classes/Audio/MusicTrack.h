#pragma once
/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * MusicTrack.h
 * 替代原 DirectX::MusicSegment 类。
 * 管理背景音乐的加载、播放、停止和循环。
 */

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

#include <string>

namespace TopSpeed
{

/**
 * MusicTrack
 *
 * 替代原 DirectX::MusicSegment 类。
 * 封装单个背景音乐轨道的完整生命周期管理。
 *
 * 原始对应关系：
 *   DirectX::MusicSegment          → TopSpeed::MusicTrack
 *   IDirectMusicPerformance8       → cocos2d::experimental::AudioEngine
 *   IDirectMusicSegment8           → AudioEngine audioID
 *   MusicSegment::download()       → MusicTrack::preload()
 *   MusicSegment::play()           → MusicTrack::play()
 *   MusicSegment::stop()           → MusicTrack::stop()
 *   MusicSegment::isPlaying()      → MusicTrack::isPlaying()
 *   MusicSegment::repeats()        → MusicTrack::setRepeats()
 */
class MusicTrack
{
public:
    /**
     * 构造函数。
     * 对应原 MusicSegment 构造（传入 performance、loader、segment）。
     *
     * @param filename  音频文件路径（支持 OGG/MP3）
     */
    explicit MusicTrack(const std::string& filename);

    /**
     * 析构函数，停止播放并释放资源。
     */
    ~MusicTrack();

    /**
     * 预加载音频文件到内存。
     * 对应原 MusicSegment::download()（下载音频数据到合成器）。
     *
     * 调用 AudioEngine::preload() 将文件缓存到内存，
     * 避免 play() 时的 I/O 延迟（对实时游戏音频非常重要）。
     */
    void preload();

    /**
     * 播放音乐轨道。
     * 对应原 MusicSegment::play(looped, secondary, audioPath)。
     *
     * @param looped  是否循环播放（默认 true，背景音乐通常需要循环）
     *
     * 注意：如果 setRepeats() 设置了有限循环次数（> 0），
     *       则 looped 参数将被忽略，通过 finishCallback 计数实现。
     */
    void play(bool looped = true);

    /**
     * 停止播放。
     * 对应原 MusicSegment::stop(immediately)。
     *
     * @param immediately  是否立即停止（默认 true）
     *
     * 注意：原 DirectMusic 支持 DMUS_SEGF_MEASURE（等到小节结束后停止），
     *       Cocos2d-x AudioEngine 暂不支持此功能，始终立即停止。
     *       如需实现渐出效果，可在停止前手动调用音量淡出动画。
     */
    void stop(bool immediately = true);

    /**
     * 检查音乐是否正在播放。
     * 对应原 MusicSegment::isPlaying()。
     *
     * @return  true = 正在播放
     */
    bool isPlaying() const;

    /**
     * 设置音量（0~100）。
     * 对应原（通过 MusicManager::volume() 统一控制，此处为单轨控制）。
     *
     * @param value  音量值，范围 0（静音）~ 100（最大）
     */
    void setVolume(int value);

    /**
     * 获取当前音量（0~100）。
     */
    int getVolume() const;

    /**
     * 设置循环重复次数。
     * 对应原 MusicSegment::repeats(Int times)。
     *
     * @param times  循环次数：
     *               -1 = 无限循环（对应原 DMUS_SEG_REPEAT_INFINITE）
     *                0 = 播放一次后停止
     *               >0 = 播放 times 次后停止
     *
     * 注意：Cocos2d-x AudioEngine 仅支持 looped=true/false，
     *       有限次重复（times > 0）需在 AudioEngine::setFinishCallback
     *       中自行计数实现：每次播放完毕后判断是否达到次数，
     *       未达到则再次调用 play2d()。
     */
    void setRepeats(int times);

    /**
     * 获取当前设置的循环重复次数。
     */
    int getRepeats() const;

    /**
     * 获取音频文件名。
     */
    const std::string& getFilename() const;

private:
    // 内部播放方法（处理 repeats 计数逻辑）
    void playInternal();

    std::string m_filename;
    int         m_audioId;      // 当前 AudioEngine audioID，INVALID_AUDIO_ID = 未播放
    int         m_volume;       // 0~100
    int         m_repeats;      // -1=无限, 0=一次, >0=指定次数
    int         m_playCount;    // 当前已播放次数（用于 repeats > 0 的计数）
    bool        m_preloaded;    // 是否已预加载
};

} // namespace TopSpeed
