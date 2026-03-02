#pragma once
/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * SoundEffect.h
 * 替代原 DirectX::Sound 类。
 * 支持多缓冲并发播放（同一音效可同时播放多次，如碰撞音），
 * 以及 3D 空间音效（通过 OpenAL AL_POSITION 实现）。
 */

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

#include <string>
#include <vector>

namespace TopSpeed
{

/**
 * SoundEffect
 *
 * 替代原 DirectX::Sound 类。
 * 通过维护 nBuffers 个独立的 AudioEngine audioID 槽，
 * 模拟原 DirectSound 的多缓冲并发播放机制。
 *
 * 原始对应关系：
 *   DirectX::Sound          → TopSpeed::SoundEffect
 *   IDirectSoundBuffer[]    → m_slots[]（audioID 数组）
 *   Sound::getFreeBuffer()  → SoundEffect::getFreeSlot()
 */
class SoundEffect
{
public:
    /**
     * 构造函数。
     * 对应原 Sound 类构造（传入 SoundManager、filename、nBuffers 等）。
     *
     * @param filename   音频文件路径（支持 OGG/WAV）
     * @param enable3d   是否启用 3D 空间音效（影响 setPosition3D 是否生效）
     * @param nBuffers   并发缓冲槽数量，对应原 Sound 的多缓冲并发数
     *                   （默认 4，即同一音效最多同时播放 4 次）
     */
    SoundEffect(const std::string& filename, bool enable3d = false, int nBuffers = 4);

    /**
     * 析构函数，停止所有播放并释放 AudioEngine 资源。
     */
    ~SoundEffect();

    /**
     * 播放音效。
     * 对应原 Sound::play(priority, looped)。
     *
     * 内部调用 getFreeSlot() 找到空闲槽，在该槽中播放音效，
     * 并通过 AudioEngine::setFinishCallback 在播放完毕后自动清除槽。
     *
     * @param looped  是否循环播放
     * @return        使用的槽索引，-1 表示所有槽均被占用（播放失败）
     */
    int play(bool looped = false);

    /**
     * 停止所有正在播放的实例。
     * 对应原 Sound::stop()。
     */
    void stop();

    /**
     * 是否有任意实例正在播放。
     * 对应原 Sound::playing()。
     *
     * @return  true = 至少有一个槽正在播放
     */
    bool isPlaying() const;

    /**
     * 设置音量（影响所有槽）。
     * 对应原 Sound::volume(Int)。
     *
     * @param value  音量值，范围 0（静音）~ 100（最大）
     */
    void setVolume(int value);

    /**
     * 获取当前音量（0~100）。
     */
    int getVolume() const;

    /**
     * 设置声像（立体声平衡）。
     * 对应原 Sound::pan(Int)。
     *
     * @param value  声像值，范围 -100（纯左）~ 0（居中）~ +100（纯右）
     *
     * 注意：Cocos2d-x AudioEngine 不直接支持声像控制。
     *       需要通过 OpenAL 扩展实现：
     *       将声源放置在 2D 平面上，X 轴表示左右偏移：
     *         alSource3f(sourceId, AL_POSITION, panValue, 0.0f, 0.0f);
     *       其中 panValue = (value / 100.0f)，并设置 AL_SOURCE_RELATIVE = AL_TRUE。
     *       如果 AudioManager::reverseStereo 为 true，则 panValue 取反。
     */
    void setPan(int value);

    /**
     * 设置播放音调（变速不变调需 OpenAL 扩展）。
     * 对应原 Sound::frequency(Int)（原为 Hz，此处改为音调倍率）。
     *
     * @param pitch  音调倍率，1.0f = 原始，2.0f = 高八度，0.5f = 低八度
     *
     * 注意：Cocos2d-x AudioEngine 不直接支持 pitch 控制。
     *       需要通过 OpenAL 实现：
     *         alSourcef(sourceId, AL_PITCH, pitch);
     *       其中 sourceId 可通过 Cocos2d-x 内部 AudioEngine 实现层获取。
     */
    void setPitch(float pitch);

    /**
     * 设置声源的 3D 世界坐标位置（用于 HRTF 空间音效）。
     * 对应原 Sound::position(Vector3)。
     *
     * @param pos  声源位置（世界坐标系）
     *
     * 注意：需要 OpenAL 实现：
     *         alSource3f(sourceId, AL_POSITION, pos.x, pos.y, pos.z);
     *       同时确保 AudioManager::setListenerPosition() 已正确设置。
     *       启用 HRTF（alsoft.ini）后，玩家可通过耳机感知声源方向，
     *       这对视障玩家判断赛车来向至关重要。
     *
     * TODO: 实现完整的 OpenAL 3D 位置更新，包括：
     *   1. 调用 alSource3f(sourceId, AL_POSITION, x, y, z)
     *   2. 设置 AL_SOURCE_RELATIVE = AL_FALSE（世界坐标）
     *   3. 配合 AudioManager 的监听器位置/朝向以获得正确的方向感知
     */
    void setPosition3D(const cocos2d::Vec3& pos);

    /**
     * 重置音效（停止所有槽并清除状态）。
     * 对应原 Sound::reset()。
     */
    void reset();

    /**
     * 获取音频文件名。
     */
    const std::string& getFilename() const;

private:
    /**
     * 查找空闲的播放槽。
     * 对应原 Sound::getFreeBuffer()。
     *
     * 检查每个槽：
     *   - INVALID_AUDIO_ID 表示槽未被使用
     *   - AudioState::PLAYING 表示槽正在使用
     * 返回第一个空闲槽的索引，全部占用时返回 -1。
     *
     * @return  空闲槽索引，-1 = 无空闲槽
     */
    int getFreeSlot() const;

    std::string m_filename;
    bool        m_enable3d;
    int         m_nBuffers;
    int         m_volume;    // 0~100
    int         m_pan;       // -100~+100
    float       m_pitch;     // 音调倍率

    cocos2d::Vec3 m_position3d;

    // 播放槽数组：每个元素为 AudioEngine audioID
    // INVALID_AUDIO_ID 表示该槽空闲
    std::vector<int> m_slots;
};

} // namespace TopSpeed
