<div align="center">
  <img width="96" src="assets/icon_128.gif" alt="lessampler logo">
  <h1>lessampler-ng</h1>
  <p><strong>Modern, Multi-Platform Singing Voice Synthesis Resampler Engine</strong></p>
</div>

---

## Overview

**lessampler-ng** is a high-performance, modular singing voice synthesis resampler engine designed for **UTAU** and **OpenUtau**. Powered by the **WORLD vocoder** framework (Harvest, Dio, CheapTrick, D4C), it provides high-fidelity pitch-shifting, time-stretching, dynamic gain normalization (AutoAMP), Base64 pitch-bend interpolation, and an interactive **Terminal User Interface (TUI)** built with FTXUI.

---

## Features

- **WORLD Vocoder Pipeline**:
  - Pitch estimation algorithms: **Harvest** (high precision) and **Dio** (high speed).
  - Spectral envelope extraction: **CheapTrick**.
  - Aperiodicity analysis: **D4C**.
- **Interactive TUI**:
  - Multi-tab configuration manager.
  - Native file picker dialogs for voicebanks and render paths.
  - Live log inspection and execution statistics.
- **High-Speed Binary Cache**:
  - Automatically caches pre-analyzed acoustic data into `.lessaudio` files.
- **Seamless UTAU & OpenUtau Integration**:
  - Full support for 12 standard CLI resampler arguments, velocity scaling, tempo maps, and pitch bends.
- **Cross-Platform Compatibility**:
  - Native support for Linux (x86_64), macOS (Apple Silicon & Intel), and Windows (x86_64).

---

## Architecture Pipeline

```text
[Input WAV] 
    │
    ▼
[AudioModel Analysis / .lessaudio Cache] 
    │ (Harvest/Dio F0 + CheapTrick Spec + D4C AP)
    ▼
[AudioProcess: Pitch Shift & Time Stretch]
    │ (Velocity, Pitch Bend, Consonant Preservation)
    ▼
[Synthesis: WORLD Waveform Reconstruction]
    │
    ▼
[AutoAMP: Dynamic Normalization]
    │
    ▼
[Rendered Output WAV]
```

---

## CLI Usage (UTAU / OpenUtau)

```bash
lessampler <input.wav> <output.wav> <pitch> <velocity> [flags] [offset] [length] [fixed_length] [end_blank] [volume] [tempo] [pitch_bend]
```

### Argument Reference

| Index | Name | Description | Example |
| :---: | :--- | :--- | :--- |
| `1` | `input_wav` | Path to source voicebank sample | `voicebank/a.wav` |
| `2` | `output_wav` | Destination rendered WAV path | `temp/temp_0001.wav` |
| `3` | `pitch` | Target musical note or frequency | `C4`, `G#4` |
| `4` | `velocity` | Consonant speed / time stretching factor | `100` |
| `5` | `flags` | Resampler effect flags | `g-5`, `B50` |
| `6` | `offset` | Start offset in milliseconds | `0` |
| `7` | `length` | Required output duration in milliseconds | `1000` |
| `8` | `fixed_length` | Fixed consonant duration in milliseconds | `150` |
| `9` | `end_blank` | Cutoff duration from end of sample (ms) | `0` |
| `10` | `volume` | Output volume percentage (0 - 200%) | `100` |
| `11` | `tempo` | Rendering tempo with `!` prefix | `!120` |
| `12` | `pitch_bend` | Base64 pitch bend curve string | `!120AA#...` |

---

## .lessaudio File Format Specification

The `.lessaudio` format stores pre-computed acoustic features to eliminate redundant vocoder analysis during playback.

| Index | Field | Type | Description |
| :---: | :--- | :--- | :--- |
| 1 | `lessaudio_header` | `std::string` | Format identifier (`5402`) |
| 2 | `x_length` | `int` | Number of audio samples |
| 3 | `fs` | `int` | Sample rate (Hz) |
| 4 | `frame_period` | `double` | Frame shift period (ms, default: `5.0`) |
| 5 | `f0_length` | `int` | Number of temporal F0 frames |
| 6 | `w_length` | `int` | FFT frequency bin count (`fft_size / 2 + 1`) |
| 7 | `fft_size` | `int` | FFT analysis size (default: `1024`) |
| 8 | `f0` | `vector<double>` | Fundamental frequency contour (Hz) |
| 9 | `spectrogram` | `vector<vector<double>>` | Spectral envelope matrix |
| 10 | `aperiodicity` | `vector<vector<double>>` | Aperiodicity ratio matrix |

---

## Configuration Settings

Configurations are stored in `lessampler.ini`:

| Setting | Default | Description |
| :--- | :---: | :--- |
| `f0_mode` | `1` (*Harvest*) | `1` = Harvest (High Precision), `2` = Dio (High Speed) |
| `model_amp` | `0.85` | Amplitude multiplier before analysis |
| `fft_size` | `1024` | FFT window size |
| `ap_threshold` | `0.10` | D4C aperiodicity threshold |
| `f0_dio_floor` | `40.0` | Minimum pitch frequency for Dio (Hz) |
| `f0_harvest_floor`| `40.0` | Minimum pitch frequency for Harvest (Hz) |
| `f0_cheap_trick_floor` | `71.0` | Lower bound frequency for CheapTrick (Hz) |
| `debug_mode` | `false` | Enable verbose logging |

---

## Credits & Special Thanks

- **Original Project**: [@YuzukiTsuru](https://github.com/YuzukiTsuru)
- **Contributors**: [@shine5402](https://github.com/shine5402), [@hyperzlib](https://github.com/hyperzlib)
- **WORLD Vocoder**: [Masanori Morise](https://github.com/mmorise/World)
- **FTXUI**: [Arthur Sonzogni](https://github.com/ArthurSonzogni/FTXUI)
- **Portable File Dialogs**: [Sam Hocevar](https://github.com/samhocevar/portable-file-dialogs)
