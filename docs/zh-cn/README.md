<div align="center">
  <img width="96" src="assets/icon_128.gif" alt="lessampler logo">
  <h1>lessampler-ng</h1>
  <p><strong>现代化跨平台歌声合成重采样器引擎 (Singing Voice Synthesis Engine)</strong></p>
</div>

---

## 项目简介

**lessampler-ng** 是专为 **UTAU** 和 **OpenUtau** 设计的高性能、模块化歌声合成采样器引擎。它基于 **WORLD 声码器**（Harvest、Dio、CheapTrick、D4C）开发，支持高精度音高变换、时间拉伸、动态增益归一化 (AutoAMP)、Base64 音高弯曲渲染，并提供基于 FTXUI 的交互式终端界面 (TUI)。

---

## 核心特性

- **WORLD 声码器支持**：
  - 基频估计：**Harvest**（高精度）与 **Dio**（高速度）。
  - 频谱包络提取：**CheapTrick**。
  - 非周期性分析：**D4C**。
- **FTXUI 终端界面 (TUI)**：
  - 多分页参数配置。
  - 原生文件选择器支持。
  - 实时日志查看与合成测试。
- **高速二进制缓存**：
  - 自动预生成 `.lessaudio` 格式声学特征缓存。
- **全平台支持**：
  - 支持 Linux (x86_64)、macOS (Apple Silicon & Intel) 以及 Windows (x86_64)。

---

## UTAU / OpenUtau 命令行调用规范

```bash
lessampler <input.wav> <output.wav> <pitch> <velocity> [flags] [offset] [length] [fixed_length] [end_blank] [volume] [tempo] [pitch_bend]
```

### 参数说明

| 序号 | 参数名 | 描述 | 示例 |
| :---: | :--- | :--- | :--- |
| `1` | `input_wav` | 输入源音频路径 | `voicebank/a.wav` |
| `2` | `output_wav` | 输出目标音频路径 | `temp/temp_0001.wav` |
| `3` | `pitch` | 目标音高音符或频率 | `C4`, `G#4` |
| `4` | `velocity` | 辅音速度 / 时间伸缩比例 | `100` |
| `5` | `flags` | 渲染 Flags 标记 | `g-5`, `B50` |
| `6` | `offset` | 起始偏移（毫秒） | `0` |
| `7` | `length` | 目标持续时长（毫秒） | `1000` |
| `8` | `fixed_length` | 固定辅音长度（毫秒） | `150` |
| `9` | `end_blank` | 尾部截断（毫秒） | `0` |
| `10` | `volume` | 音量百分比 (0 - 200%) | `100` |
| `11` | `tempo` | 渲染速度（以 `!` 开头） | `!120` |
| `12` | `pitch_bend` | Base64 格式音高曲线数据 | `!120AA#...` |
