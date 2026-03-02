/**
 * TopSpeed — Cocos2d-x 音频迁移
 * Copyright 2003-2013 Playing in the Dark (http://playinginthedark.net)
 * Code contributors: Davy Kager, Davy Loots and Leonard de Ruijter
 * This program is distributed under the terms of the GNU General Public License version 3.
 *
 * AudioManager.cpp
 * 替代原 DirectX::SoundManager 和 DirectX::MusicManager。
 *
 * ============================================================
 * OpenAL Soft HRTF 配置说明（视障玩家 3D 空间音频关键配置）：
 *
 * 将 alsoft.ini 放置于游戏可执行文件同目录下，内容如下：
 *
 *   [general]
 *   hrtf = true
 *   hrtf-mode = full
 *
 *   [hrtf]
 *   default-hrtf = Default HRTF
 *
 * 启用 HRTF 后，玩家可通过耳机感知声源的三维空间位置（前后左右上下）。
 * 这对视障玩家判断赛车位置、障碍物方向至关重要。
 *
 * 完整 OpenAL 3D 监听器 API 示例（需 #include <AL/al.h>）：
 *
 *   // 设置监听器位置
 *   alListener3f(AL_POSITION, x, y, z);
 *
 *   // 设置监听器朝向（前向量 + 上向量，共 6 个浮点数）
 *   ALfloat orientation[6] = { fwdX, fwdY, fwdZ, upX, upY, upZ };
 *   alListenerfv(AL_ORIENTATION, orientation);
 *
 *   // 设置监听器速度（用于多普勒效应）
 *   alListener3f(AL_VELOCITY, vx, vy, vz);
 *
 * ============================================================
 */

#include "AudioManager.h"
#include "SoundEffect.h"
#include "MusicTrack.h"

using namespace cocos2d;
using namespace cocos2d::experimental;

namespace TopSpeed
{

AudioManager* AudioManager::s_instance = nullptr;

AudioManager* AudioManager::getInstance()
{
    if (!s_instance)
    {
        s_instance = new AudioManager();
    }
    return s_instance;
}

void AudioManager::destroyInstance()
{
    if (s_instance)
    {
        s_instance->releaseAll();
        delete s_instance;
        s_instance = nullptr;
    }
}

AudioManager::AudioManager()
    : m_masterVolume(1.0f)
    , m_reverseStereo(false)
    , m_listenerPosition(Vec3::ZERO)
    , m_listenerForward(Vec3(0.0f, 0.0f, -1.0f))
    , m_listenerUp(Vec3(0.0f, 1.0f, 0.0f))
{
    CCLOG("(+) AudioManager");
    // 初始化 AudioEngine（基于 OpenAL）
    AudioEngine::lazyInit();
}

AudioManager::~AudioManager()
{
    CCLOG("(-) AudioManager");
    // AudioEngine::end() 由 Cocos2d-x 应用层统一调用，此处不重复调用
}

SoundEffect* AudioManager::createEffect(const std::string& filename, bool enable3d)
{
    // 对应原 SoundManager::create()
    // 先查缓存
    std::map<std::string, SoundEffect*>::iterator it = m_effectCache.find(filename);
    if (it != m_effectCache.end())
    {
        return it->second;
    }

    // 使用 nothrow 构造，使空指针检查有效（Cocos2d-x 惯用模式）
    SoundEffect* effect = new (std::nothrow) SoundEffect(filename, enable3d);
    if (!effect)
    {
        CCLOGERROR("(!) AudioManager::createEffect : failed to create SoundEffect for %s", filename.c_str());
        return nullptr;
    }

    m_effectCache[filename] = effect;
    CCLOG("AudioManager::createEffect : created effect for %s", filename.c_str());
    return effect;
}

MusicTrack* AudioManager::createMusic(const std::string& filename)
{
    // 对应原 MusicManager::createSegmentFromFile()
    // 先查缓存
    std::map<std::string, MusicTrack*>::iterator it = m_musicCache.find(filename);
    if (it != m_musicCache.end())
    {
        return it->second;
    }

    // 使用 nothrow 构造，使空指针检查有效（Cocos2d-x 惯用模式）
    MusicTrack* track = new (std::nothrow) MusicTrack(filename);
    if (!track)
    {
        CCLOGERROR("(!) AudioManager::createMusic : failed to create MusicTrack for %s", filename.c_str());
        return nullptr;
    }

    m_musicCache[filename] = track;
    CCLOG("AudioManager::createMusic : created track for %s", filename.c_str());
    return track;
}

void AudioManager::setMasterVolume(float volume)
{
    // 对应原 MusicManager::volume(UInt volume)
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    m_masterVolume = volume;
    // 影响新播放的实例
    AudioEngine::setDefaultVolume(volume);
    // 更新所有已缓存音效的音量
    for (std::map<std::string, SoundEffect*>::iterator it = m_effectCache.begin();
         it != m_effectCache.end(); ++it)
    {
        if (it->second)
        {
            it->second->setVolume((int)(volume * 100.0f));
        }
    }
    // 更新所有已缓存音乐轨道的音量
    for (std::map<std::string, MusicTrack*>::iterator it = m_musicCache.begin();
         it != m_musicCache.end(); ++it)
    {
        if (it->second)
        {
            it->second->setVolume((int)(volume * 100.0f));
        }
    }
    CCLOG("AudioManager::setMasterVolume : %.2f", volume);
}

float AudioManager::getMasterVolume() const
{
    return m_masterVolume;
}

void AudioManager::setReverseStereo(bool reverse)
{
    // 对应原 SoundManager::reverseStereo 属性
    m_reverseStereo = reverse;
    CCLOG("AudioManager::setReverseStereo : %d", (int)reverse);
    // TODO: 通知所有已创建的 SoundEffect 更新声像（Pan）设置。
    //       在 OpenAL 中，立体声反转可通过将 AL_POSITION 的 X 轴取反来实现：
    //       alSource3f(sourceId, AL_POSITION, -x, y, z)
}

bool AudioManager::getReverseStereo() const
{
    return m_reverseStereo;
}

void AudioManager::setListenerPosition(const Vec3& position)
{
    // 对应原 Listener3D::position(Vector3)
    m_listenerPosition = position;

    // TODO: 直接调用 OpenAL API 更新监听器位置（需要 #include <AL/al.h>）：
    //   alListener3f(AL_POSITION, position.x, position.y, position.z);
    //
    // 注意：Cocos2d-x AudioEngine 不暴露 OpenAL 监听器接口，
    //       需要在项目中链接 OpenAL 并直接调用 al* 系列函数。
    //       对视障玩家而言，监听器位置直接影响 HRTF 空间感知效果。

    CCLOG("AudioManager::setListenerPosition : (%.2f, %.2f, %.2f)",
          position.x, position.y, position.z);
}

void AudioManager::setListenerOrientation(const Vec3& forward, const Vec3& up)
{
    // 对应原 Listener3D::orientation(Vector3 forward, Vector3 up)
    m_listenerForward = forward;
    m_listenerUp      = up;

    // TODO: 直接调用 OpenAL API 更新监听器朝向（需要 #include <AL/al.h>）：
    //   ALfloat orientation[6] = {
    //       forward.x, forward.y, forward.z,
    //       up.x,      up.y,      up.z
    //   };
    //   alListenerfv(AL_ORIENTATION, orientation);
    //
    // 对视障玩家而言，正确的朝向是 HRTF 感知前后方向声源的关键。
    // 当监听器朝向与车辆行驶方向一致时，玩家才能准确判断前方/后方来车声。

    CCLOG("AudioManager::setListenerOrientation : forward(%.2f,%.2f,%.2f) up(%.2f,%.2f,%.2f)",
          forward.x, forward.y, forward.z,
          up.x, up.y, up.z);
}

void AudioManager::releaseAll()
{
    CCLOG("AudioManager::releaseAll");

    // 停止并释放所有音效
    for (std::map<std::string, SoundEffect*>::iterator it = m_effectCache.begin();
         it != m_effectCache.end(); ++it)
    {
        if (it->second)
        {
            it->second->stop();
            delete it->second;
        }
    }
    m_effectCache.clear();

    // 停止并释放所有音乐轨道
    for (std::map<std::string, MusicTrack*>::iterator it = m_musicCache.begin();
         it != m_musicCache.end(); ++it)
    {
        if (it->second)
        {
            it->second->stop();
            delete it->second;
        }
    }
    m_musicCache.clear();
}

void AudioManager::update(float dt)
{
    // 每帧更新，由 Cocos2d-x Scheduler 调用：
    //   Director::getInstance()->getScheduler()->schedule(
    //       CC_SCHEDULE_SELECTOR(AudioManager::update),
    //       AudioManager::getInstance(), 0, false);
    //
    // 当前暂无需要每帧处理的逻辑。
    // 未来可在此处理：音效淡入淡出、监听器跟随摄像机、音频状态轮询等。
    (void)dt;
}

} // namespace TopSpeed
