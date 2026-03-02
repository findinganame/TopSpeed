#pragma once
/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * AudioManager.h
 * 替代原 DirectX::SoundManager 和 DirectX::MusicManager，实现为单例。
 * 统一管理所有音效（SoundEffect）和音乐轨道（MusicTrack）的生命周期。
 */

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

#include <string>
#include <map>
#include <vector>

// 前向声明
namespace TopSpeed
{
    class SoundEffect;
    class MusicTrack;
}

namespace TopSpeed
{

/**
 * AudioManager
 *
 * 单例音频管理器，替代原 DirectX::SoundManager + DirectX::MusicManager。
 * 负责：
 *  - SoundEffect / MusicTrack 的缓存与创建
 *  - 主音量控制
 *  - OpenAL 监听器（Listener）位置与朝向设置（3D 空间音频）
 *  - 每帧更新
 */
class AudioManager
{
public:
    /**
     * 获取单例实例。
     * 对应原 SoundManager / MusicManager 的全局实例。
     */
    static AudioManager* getInstance();

    /**
     * 销毁单例实例，释放所有资源。
     * 在游戏退出时调用。
     */
    static void destroyInstance();

    /**
     * 创建或从缓存获取一个音效对象。
     * 对应原 SoundManager::create()。
     *
     * @param filename  音频文件路径（支持 OGG/WAV）
     * @param enable3d  是否启用 3D 空间音效（OpenAL 位置属性）
     * @return          SoundEffect 指针（由 AudioManager 持有所有权），失败返回 nullptr
     */
    SoundEffect* createEffect(const std::string& filename, bool enable3d = false);

    /**
     * 创建或从缓存获取一个音乐轨道对象。
     * 对应原 MusicManager::createSegmentFromFile()。
     *
     * @param filename  音频文件路径（支持 OGG/MP3）
     * @return          MusicTrack 指针（由 AudioManager 持有所有权），失败返回 nullptr
     */
    MusicTrack* createMusic(const std::string& filename);

    /**
     * 设置主音量（影响所有通过 AudioEngine 播放的音频）。
     * 对应原 MusicManager::volume(UInt)。
     *
     * @param volume  音量值，范围 0.0f（静音）~ 1.0f（最大）
     */
    void setMasterVolume(float volume);

    /**
     * 获取当前主音量。
     * @return 音量值，范围 0.0f ~ 1.0f
     */
    float getMasterVolume() const;

    /**
     * 设置是否反转立体声左右声道。
     * 对应原 SoundManager::reverseStereo 属性。
     *
     * @param reverse  true = 左右互换
     *
     * 注意：Cocos2d-x AudioEngine 本身不支持此功能，
     *       需要在 OpenAL 层通过 AL_POSITION 的 X 轴取反来模拟。
     *       此标志将传递给所有新创建的 SoundEffect。
     */
    void setReverseStereo(bool reverse);

    /**
     * 获取当前立体声反转状态。
     */
    bool getReverseStereo() const;

    /**
     * 设置 OpenAL 监听器（Listener）的世界坐标位置。
     * 对应原 Listener3D::position(Vector3)。
     *
     * 3D 空间音频（HRTF）核心接口：
     *   alListener3f(AL_POSITION, x, y, z)
     *
     * @param position  监听器位置（世界坐标系）
     *
     * TODO: 调用 OpenAL API alListener3f(AL_POSITION, x, y, z) 以更新监听器位置。
     *       在 Cocos2d-x 中，AudioEngine 不直接暴露此接口，
     *       需通过 #include <AL/al.h> 直接调用 OpenAL。
     */
    void setListenerPosition(const cocos2d::Vec3& position);

    /**
     * 设置 OpenAL 监听器的朝向（前向量 + 上向量）。
     * 对应原 Listener3D::orientation(Vector3 forward, Vector3 up)。
     *
     * 3D 空间音频（HRTF）核心接口：
     *   ALfloat orientation[6] = { fwdX, fwdY, fwdZ, upX, upY, upZ };
     *   alListenerfv(AL_ORIENTATION, orientation);
     *
     * @param forward  前向单位向量
     * @param up       上方单位向量
     *
     * TODO: 调用 alListenerfv(AL_ORIENTATION, orientation) 以更新监听器朝向。
     *       对视障玩家而言，正确的 HRTF 朝向是感知声源方位的关键。
     */
    void setListenerOrientation(const cocos2d::Vec3& forward, const cocos2d::Vec3& up);

    /**
     * 释放所有已缓存的 SoundEffect 和 MusicTrack 对象。
     * 通常在场景切换或游戏退出时调用。
     */
    void releaseAll();

    /**
     * 每帧更新，由游戏主循环调用（Scheduler）。
     * 可在此处理音频状态轮询、回调等。
     *
     * @param dt  帧时间间隔（秒）
     */
    void update(float dt);

private:
    AudioManager();
    ~AudioManager();

    // 禁止拷贝和赋值
    AudioManager(const AudioManager&);
    AudioManager& operator=(const AudioManager&);

    static AudioManager* s_instance;

    float m_masterVolume;
    bool  m_reverseStereo;

    cocos2d::Vec3 m_listenerPosition;
    cocos2d::Vec3 m_listenerForward;
    cocos2d::Vec3 m_listenerUp;

    // 音效缓存：文件名 → SoundEffect*
    std::map<std::string, SoundEffect*> m_effectCache;

    // 音乐缓存：文件名 → MusicTrack*
    std::map<std::string, MusicTrack*> m_musicCache;
};

} // namespace TopSpeed
