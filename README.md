# CarAudioEngine — 车载多音区音频策略与混音路由仿真引擎

基于 C++20 构建的车载智能座舱音频系统仿真引擎，涵盖 **多音区路由（4 音区 + 主驾头枕 + 全车安全总线）**、**类 AAOS 音频焦点仲裁（Focus/Ducking/全局抢占）** 以及 **完整车载 DSP 管线（重采样、平滑增益、Fade/Balance、5 段 PEQ、随速音量补偿 SDVC、防削波 Limiter）**。

## 目录结构

- `include/car_audio/` & `src/`：核心引擎库 `car_audio_core` 与 CLI 仿真器 `car_audio_cli`
- `config/car_audio_config.json`：音区、总线、音量组、SDVC 与 EQ 默认配置
- `config/scenarios/demo_drive_scenario.json`：典型行车多音区并发与抢占仿真场景
- `tests/`：单元测试与端到端混音/Ducking 验证套件
- `docs/ARCHITECTURE.md`：详细系统架构与模块设计文档

## 编译与运行测试

```bash
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

## 运行仿真场景（导出各总线 WAV）

```bash
./build/car_audio_cli \
  --config config/car_audio_config.json \
  --scenario config/scenarios/demo_drive_scenario.json \
  --out sim_output
```

运行后将在 `sim_output/` 下生成各物理总线的 48kHz PCM16 WAV 文件（如 `bus_driver_media.wav`、`bus_driver_headrest.wav`、`bus_rear_left_media.wav`、`bus_cabin_safety.wav`）。

## 交互式 CLI 模式

```bash
./build/car_audio_cli --config config/car_audio_config.json
```

在交互命令行中输入 `help` 可查看支持的控制指令（`play`、`stop`、`vol`、`speed`、`fb`、`eq`、`route`、`step`、`status`、`flush`）。
