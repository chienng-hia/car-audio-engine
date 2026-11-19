# CarAudioEngine 系统架构设计文档

## 1. 设计目标与定位
`CarAudioEngine` 是一个纯 C++20 实现的**车载多音区音频策略与混音路由仿真引擎**，用于在无实机硬件的 Linux 环境下完整验证汽车智能座舱音频系统的核心业务逻辑与 DSP 信号处理管线。

---

## 2. 多音区（Audio Zones）与物理总线拓扑

系统默认采用 **4 音区 + 主驾头枕私密通道 + 全车安全广播总线** 的架构（由 `config/car_audio_config.json` 驱动）：

```
+-------------------------------------------------------------------+
|                        AudioStreams (Apps / Services)              |
|  Media | Navigation | VoiceCommand | Call | ADASWarning | eCall   |
+----------------------------------+--------------------------------+
                                   |
                                   v
                      +-------------------------+
                      |  FocusManager (AAOS)    |  <-- 音区内独立仲裁 + 跨音区全局抢占
                      +------------+------------+
                                   |
                                   v
                      +-------------------------+
                      |  AudioRouter            |  <-- (Zone, Usage) -> BusId + VolumeGroup
                      +------------+------------+
                                   |
        +--------------------------+--------------------------+
        |                          |                          |
        v                          v                          v
 [bus_driver_media]      [bus_driver_headrest]      [bus_cabin_safety]
 [bus_passenger_media]    (Nav / Voice / Call)       (ADAS / eCall)
 [bus_rear_left_media]
 [bus_rear_right_media]
```

- **音区隔离**：主驾（`DRIVER`）、副驾（`PASSENGER`）、后左（`REAR_LEFT`）、后右（`REAR_RIGHT`）的娱乐媒体互相独立，互不干扰。
- **头枕私密通道**：主驾的导航播报（`NAVIGATION`）、语音助手（`VOICE_COMMAND`）及蓝牙通话（`CALL`）默认路由至 `bus_driver_headrest`，并支持通过 `set_route_override` 动态切换。
- **全车安全总线**：`ADAS_WARNING` 与 `EMERGENCY_ECALL` 路由至 `bus_cabin_safety`（`CABIN_GLOBAL`），触发跨音区全局抢占。

---

## 3. 音频焦点仲裁矩阵（FocusManager）

参考 Android Automotive OS (`CarAudioFocus`) 交互规则，按优先级从高到低排序：
1. `EMERGENCY_ECALL` (100) — 最高优先级，全局独占，拒绝一切低优先级请求
2. `ADAS_WARNING` (90) — 暂停娱乐/导航/语音，与通话并发提示
3. `CALL` (70) / `CALL_RING` (60) — 暂停同音区媒体，允许导航并发（可 Duck）
4. `VOICE_COMMAND` (50) — 暂停同音区媒体
5. `NAVIGATION` (40) / `ALARM` (30) / `NOTIFICATION` (20) — 与媒体并发，触发 `GAIN_TRANSIENT_MAY_DUCK`（媒体平滑衰减 -12 dB）
6. `MEDIA` (10) — 基础常驻焦点

当高优先级流结束并调用 `abandon_focus` 时，状态机自动重新计算剩余持有者状态，将此前 `DUCKED` 或 `PAUSED` 的媒体流平滑恢复至 `ACTIVE`。

---

## 4. 混音与车载 DSP 信号处理管线

每个音频块（默认 `480 frames = 10ms @ 48kHz`）按以下顺序处理：
1. **输入重采样 (`dsp::Resampler`)**：任意采样率与单/双声道统一转换为 48kHz Stereo Float32。
2. **平滑增益/Ducking (`dsp::GainSmoother`)**：25ms 线性斜坡过渡，消除切歌、Ducking 与静音时的爆音（Click/Pop）。
3. **音量组与随速音量补偿 (`VolumeGroup` + `dsp::Sdvc`)**：
   - 对数分贝音量曲线映射（UI step 0..40 -> dB）。
   - 车速 40~120 km/h 区间平滑提升 0~6 dB（Hermite S-curve），`safety` 总线保持确定性电平不受 SDVC 影响。
4. **声场 5 段参数化均衡 (`dsp::FiveBandEq`)**：基于 Biquad IIR（LowShelf 60Hz, Peak 250Hz/1kHz/4kHz, HighShelf 12kHz），支持 `Flat` / `Rock` / `Vocal` / `BassBoost` 预设。
5. **声场空间平衡 (`dsp::SpatialBalance`)**：支持前后 Fade（-1.0..+1.0）与左右 Balance（-1.0..+1.0）。
6. **防削波限幅 (`dsp::Limiter`)**：立体声联动 Peak Limiter，防止多路混音叠加过载失真。
7. **输出后端 (`IAudioSink`)**：当前实现 `WavFileSink` 按总线导出 16-bit PCM WAV 文件；接口已解耦，可无缝扩展 ALSA / PortAudio 实时声卡后端。
