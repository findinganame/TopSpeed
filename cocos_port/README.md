# TopSpeed — Cocos2d-x 迁移工作目录

本目录是将 TopSpeed 音频赛车游戏从 **Windows DirectX SDK** 迁移至 **Cocos2d-x** 框架的工作目录。

---

## 迁移目标

将原始代码从以下依赖中解放出来：

- DirectSound8 / DirectMusic8（已停止维护）
- DirectPlay（已废弃）
- DirectX SDK 2007（跨平台性极差）

迁移至 **Cocos2d-x 3.17+**（基于 OpenAL），实现跨平台音频支持，同时保留视障玩家所需的 3D 空间音频（HRTF）功能。

---

## Cocos2d-x 版本要求

- **建议版本**：Cocos2d-x **3.17+**
- 音频引擎使用 `cocos2d::experimental::AudioEngine`（基于 OpenAL）
- 需要 OpenAL Soft 1.19+ 以支持完整 HRTF 功能

---

## 已迁移的模块

### 音频模块 (`Classes/Audio/`)

| 新文件 | 替代原文件 | 说明 |
|--------|------------|------|
| `AudioManager.h/.cpp` | `dxcommon/Src/Sound.cpp` + `Music.cpp` | 统一音频管理单例，替代 `DirectX::SoundManager` 和 `DirectX::MusicManager` |
| `SoundEffect.h/.cpp` | `dxcommon/If/Sound.h` + `Sound.cpp` | 多缓冲音效，替代 `DirectX::Sound` |
| `MusicTrack.h/.cpp` | `dxcommon/If/Music.h` + `Music.cpp` | 背景音乐轨道，替代 `DirectX::MusicSegment` |
| `alsoft.ini` | 无对应（新增） | OpenAL Soft HRTF 配置文件 |

#### 关键特性对照

| 原 DirectX 特性 | Cocos2d-x 实现 |
|-----------------|----------------|
| `IDirectSoundBuffer` 多缓冲并发 | `AudioEngine` 多槽（slot）并发 |
| `DS3DALG_HRTF_FULL` 空间音频 | OpenAL Soft HRTF（需 `alsoft.ini`） |
| OGG Vorbis 手动解码 | `AudioEngine` 内置 OGG 支持 |
| 音量 0~100 | `AudioEngine::setVolume(0.0f~1.0f)` |
| 声像 Pan -100~+100 | OpenAL AL_POSITION（需扩展） |
| `MusicSegment::repeats()` | `AudioEngine` looped + finish callback 计数 |
| `MusicSegment::stop(immediately=false)` | 暂无小节边界停止，Cocos2d-x 仅支持立即停止 |

---

## 待迁移的模块

- [ ] **输入模块**（`dxcommon/Src/Input.cpp`）— DirectInput8 → Cocos2d-x EventKeyboard / EventGamepad
- [ ] **渲染模块**（`dxcommon/Src/D3DSprite.cpp` 等）— DirectX 3D → Cocos2d-x Scene/Node/Sprite（或纯音频模式，不需要渲染）
- [ ] **网络模块**（`dxcommon/Src/Network.cpp`）— DirectPlay → BSD Sockets / Cocos2d-x Network
- [ ] **游戏逻辑**（`vs_projects/topspeed/`）— 游戏主循环、物理、AI 等

---

## 3D 空间音频（OpenAL HRTF）配置说明

TopSpeed 是面向**视障玩家**的游戏，3D 空间音频是核心体验。

### 配置文件

将 `Classes/Audio/alsoft.ini` 复制到游戏可执行文件同目录下：

```ini
[general]
hrtf = true
hrtf-mode = full

[hrtf]
default-hrtf = Default HRTF
```

### 说明

- **HRTF（Head-Related Transfer Function）**：通过模拟人耳对不同方向声音的滤波特性，让玩家能够感知声源的三维空间位置（前后左右上下）。
- `SoundEffect::setPosition3D()` 设置声源位置（对应原 `Sound::position(Vector3)`）。
- `AudioManager::setListenerPosition()` / `setListenerOrientation()` 设置监听器（对应原 `Listener3D::position()` / `orientation()`）。
- 完整 OpenAL 3D API 调用示例见 `AudioManager.cpp` 中的注释。

---

## 原始代码版权

原始代码版权归 **Playing in the Dark**（http://playinginthedark.net）所有，
贡献者：Davy Kager、Davy Loots、Leonard de Ruijter，
基于 GNU GPL v3 授权发布。
