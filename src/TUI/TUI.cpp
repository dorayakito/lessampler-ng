/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TUI.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <portable-file-dialogs.h>

#include "AudioModel/Synthesis/Synthesis.h"
#include "AudioModel/lessAudioModel.h"
#include "AudioProcess/AudioProcess.h"
#include "AudioProcess/AutoAMP.h"
#include "ConfigUnit/ConfigUnit.h"
#include "Dialogs/Dialogs.h"
#include "FileIO/AudioModelIO.h"
#include "FileIO/GenerateAudioModel.h"
#include "FileIO/WavIO.h"
#include "Shine/Shine.h"
#include "Utils/LOG.h"
#include "Utils/Timer.h"
#include "lessconfig.h"

using namespace ftxui;

void TUI::Launch(lessConfigure &configure, const std::filesystem::path &exec_path) {
    auto screen = ScreenInteractive::Fullscreen();

    // Navigation Tabs
    std::vector<std::string> tab_titles = {
        "  📁 Model Generator  ",
        "  🎵 Test Resampler  ",
        "  ⚙️ Configuration  ",
        "  ℹ️ About & Help  ",
    };
    int tab_selected = 0;

    // --- Tab 0: Model Generator State ---
    std::string gen_folder_path = "test";
    std::string gen_status = "Ready to generate audio models for voicebank WAV files.";
    std::string gen_log = "";

    auto btn_gen_browse = Button("📂 Browse Folder...", [&] {
        std::string initial_path = gen_folder_path.empty() ? "." : gen_folder_path;
        auto dir = pfd::select_folder("Select Voicebank Folder", initial_path).result();
        if (!dir.empty()) {
            gen_folder_path = dir;
            gen_status = "Selected folder: " + dir;
        }
    });

    auto input_gen_folder = Input(&gen_folder_path, "Folder path (e.g. test or /path/to/voicebank)");

    auto btn_gen_start = Button("⚡ Start Model Generation", [&] {
        if (gen_folder_path.empty() || !std::filesystem::exists(gen_folder_path)) {
            gen_status = "❌ Error: Directory does not exist: " + gen_folder_path;
            Dialogs::notify("Error: Directory not found", "lessampler");
            return;
        }

        gen_status = "⏳ Generating models for folder: " + gen_folder_path + "...";
        auto start = std::chrono::high_resolution_clock::now();

        try {
            GenerateAudioModel genmodule(std::filesystem::path(gen_folder_path), configure);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            gen_status = "✅ Successfully generated models in " + std::to_string(duration) + " ms!";
            gen_log = "Voicebank folder: " + gen_folder_path + "\nStatus: Complete\nElapsed time: " + std::to_string(duration) + " ms";
            Dialogs::notify("Model Generation Complete!", "lessampler");
        } catch (const std::exception &e) {
            gen_status = "❌ Exception: " + std::string(e.what());
            Dialogs::notify("Error during modeling: " + std::string(e.what()), "lessampler");
        }
    });

    auto container_tab_gen = Container::Vertical({
        Container::Horizontal({
            input_gen_folder,
            btn_gen_browse,
        }),
        btn_gen_start,
    });

    // --- Tab 1: Test Resampler State ---
    std::string synth_in_wav = "test/vaiueo2d.wav";
    std::string synth_out_wav = "output_synth.wav";
    std::string synth_note = "C4";
    std::string synth_velocity = "100";
    std::string synth_length = "1000";
    std::string synth_flags = "";
    std::string synth_status = "Ready to resample audio.";
    std::string synth_log = "";

    auto btn_synth_browse_in = Button("📂 Browse Input...", [&] {
        auto sel = pfd::open_file("Select Input WAV", ".", {"Audio Files", "*.wav"}).result();
        if (!sel.empty()) {
            synth_in_wav = sel[0];
            synth_status = "Selected input: " + synth_in_wav;
        }
    });

    auto btn_synth_browse_out = Button("💾 Save As...", [&] {
        auto sel = pfd::save_file("Select Output WAV", synth_out_wav, {"Audio Files", "*.wav"}).result();
        if (!sel.empty()) {
            synth_out_wav = sel;
            synth_status = "Output path: " + synth_out_wav;
        }
    });

    auto input_synth_in = Input(&synth_in_wav, "Input WAV path");
    auto input_synth_out = Input(&synth_out_wav, "Output WAV path");
    auto input_synth_note = Input(&synth_note, "Pitch note (e.g. C4, A4, F#3)");
    auto input_synth_velocity = Input(&synth_velocity, "Velocity (0-200, default 100)");
    auto input_synth_length = Input(&synth_length, "Required Length in ms (default 1000)");
    auto input_synth_flags = Input(&synth_flags, "Flags (e.g. g-5, B50, etc.)");

    auto btn_synth_start = Button("🎶 Resample & Synthesize", [&] {
        if (!std::filesystem::exists(synth_in_wav)) {
            synth_status = "❌ Input WAV file not found: " + synth_in_wav;
            return;
        }

        try {
            synth_status = "⏳ Synthesizing note " + synth_note + "...";

            // Prepare CLI args for UTAU pipeline
            std::vector<std::string> args = {
                "lessampler",
                synth_in_wav,
                synth_out_wav,
                synth_note,
                synth_velocity,
                synth_flags,
                "0",            // offset
                synth_length,   // required length
                "0",            // fixed part
                "0",            // cutoff / end blank
                "100",          // volume
                "0",            // modulation
                "!120",         // tempo
                ""              // pitch bend
            };

            std::vector<char *> argv_ptrs;
            for (auto &arg : args) {
                argv_ptrs.push_back(const_cast<char *>(arg.data()));
            }
            int argc_sim = static_cast<int>(argv_ptrs.size());

            auto start = std::chrono::high_resolution_clock::now();

            AudioModelIO audio_model_io(synth_in_wav);
            if (!audio_model_io.CheckAudioModel(configure)) {
                GenerateAudioModel genmodule(synth_in_wav, configure);
            }
            audio_model_io.ReadAudioModel(configure);
            auto origin_audio_model = audio_model_io.GetAudioModel();

            Shine shine(argc_sim, argv_ptrs.data(), origin_audio_model, Shine::SHINE_MODE::UTAU);
            auto shine_para = shine.GetShine();

            AudioProcess audioProcess(origin_audio_model, shine_para);
            auto trans_audio_model = audioProcess.GetTransAudioModel();

            Synthesis synthesis(trans_audio_model, shine_para.output_samples);
            auto out_wav_data = synthesis.GetWavData();

            AutoAMP amp(shine_para, out_wav_data);
            out_wav_data = amp.GetAMP();

            WavIO::WriteWav(synth_out_wav, out_wav_data, shine_para.output_samples, trans_audio_model.fs);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            synth_status = "✅ Synthesis complete! Saved to: " + synth_out_wav + " (" + std::to_string(duration) + " ms)";
            synth_log = "Input: " + synth_in_wav + "\nOutput: " + synth_out_wav +
                        "\nPitch: " + synth_note + " | Velocity: " + synth_velocity +
                        "\nSample Rate: " + std::to_string(trans_audio_model.fs) + " Hz" +
                        "\nGenerated Samples: " + std::to_string(shine_para.output_samples) +
                        "\nElapsed: " + std::to_string(duration) + " ms";

            Dialogs::notify("Synthesized " + synth_note + " -> " + synth_out_wav, "lessampler");
        } catch (const std::exception &e) {
            synth_status = "❌ Synthesis failed: " + std::string(e.what());
            Dialogs::notify("Synthesis error: " + std::string(e.what()), "lessampler");
        }
    });

    auto container_tab_synth = Container::Vertical({
        Container::Horizontal({input_synth_in, btn_synth_browse_in}),
        Container::Horizontal({input_synth_out, btn_synth_browse_out}),
        Container::Horizontal({input_synth_note, input_synth_velocity, input_synth_length}),
        input_synth_flags,
        btn_synth_start,
    });

    // --- Tab 2: Settings State ---
    std::vector<std::string> f0_modes = {"Harvest (High Accuracy)", "Dio (Fast Speed)"};
    int f0_mode_selected = (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_DIO) ? 1 : 0;
    bool debug_mode = configure.debug_mode;
    std::string amp_str = std::to_string(configure.model_amp);
    std::string fft_size_str = std::to_string(configure.fft_size);
    std::string ap_threshold_str = std::to_string(configure.ap_threshold);
    std::string settings_status = "Configure lessampler synthesis & analysis parameters.";

    auto check_debug = Checkbox("Enable Debug Output Logging", &debug_mode);
    auto radio_f0 = Radiobox(&f0_modes, &f0_mode_selected);
    auto input_amp = Input(&amp_str, "Model Amplitude (0.0 - 1.0)");
    auto input_fft = Input(&fft_size_str, "FFT Size (default 1024)");
    auto input_ap = Input(&ap_threshold_str, "Aperiodicity Threshold (default 0.10)");

    auto btn_apply_settings = Button("💾 Apply & Save Settings", [&] {
        try {
            configure.debug_mode = debug_mode;
            configure.f0_mode = (f0_mode_selected == 1) ? lessConfigure::F0_MODE::F0_MODE_DIO : lessConfigure::F0_MODE::F0_MODE_HARVEST;
            configure.model_amp = std::stod(amp_str);
            configure.fft_size = std::stoi(fft_size_str);
            configure.ap_threshold = std::stod(ap_threshold_str);
            settings_status = "✅ Settings applied for current session!";
            Dialogs::notify("Configuration applied successfully", "lessampler");
        } catch (const std::exception &e) {
            settings_status = "❌ Invalid parameter input: " + std::string(e.what());
        }
    });

    auto container_tab_settings = Container::Vertical({
        check_debug,
        radio_f0,
        Container::Horizontal({input_amp, input_fft, input_ap}),
        btn_apply_settings,
    });

    // --- Tab 3: About & Help State ---
    auto btn_exit = Button("🚪 Exit TUI", screen.ExitLoopClosure());

    auto container_tab_about = Container::Vertical({
        btn_exit,
    });

    // Main Tab Container
    auto tab_container = Container::Tab({
        container_tab_gen,
        container_tab_synth,
        container_tab_settings,
        container_tab_about,
    }, &tab_selected);

    auto menu_tabs = Menu(&tab_titles, &tab_selected, MenuOption::Horizontal());

    auto main_container = Container::Vertical({
        Container::Horizontal({
            menu_tabs,
            btn_exit,
        }),
        tab_container,
    });

    auto renderer = Renderer(main_container, [&] {
        Element content_element;

        if (tab_selected == 0) {
            content_element = vbox({
                text(" VoiceBank Audio Model Generator ") | bold | color(Color::Cyan),
                separator(),
                hbox({
                    text("Voicebank Directory: ") | bold,
                    input_gen_folder->Render() | flex | border,
                    btn_gen_browse->Render(),
                }),
                separator(),
                btn_gen_start->Render() | center,
                separator(),
                text("Status: " + gen_status) | bold | color(Color::Yellow),
                gen_log.empty() ? filler() : (vbox({
                    text("Log Output:") | bold,
                    text(gen_log) | dim,
                }) | border | flex),
            });
        } else if (tab_selected == 1) {
            content_element = vbox({
                text(" Test Resampler & UTAU Synthesizer ") | bold | color(Color::GreenLight),
                separator(),
                hbox({
                    text("Input WAV:  ") | bold,
                    input_synth_in->Render() | flex | border,
                    btn_synth_browse_in->Render(),
                }),
                hbox({
                    text("Output WAV: ") | bold,
                    input_synth_out->Render() | flex | border,
                    btn_synth_browse_out->Render(),
                }),
                hbox({
                    vbox({text("Pitch Note:") | bold, input_synth_note->Render() | border}) | flex,
                    vbox({text("Velocity:") | bold, input_synth_velocity->Render() | border}) | flex,
                    vbox({text("Length (ms):") | bold, input_synth_length->Render() | border}) | flex,
                }),
                hbox({
                    text("Flags: ") | bold,
                    input_synth_flags->Render() | flex | border,
                }),
                separator(),
                btn_synth_start->Render() | center,
                separator(),
                text("Status: " + synth_status) | bold | color(Color::Yellow),
                synth_log.empty() ? filler() : (vbox({
                    text("Synthesis Details:") | bold,
                    text(synth_log) | dim,
                }) | border | flex),
            });
        } else if (tab_selected == 2) {
            content_element = vbox({
                text(" Engine Configuration & Analysis Settings ") | bold | color(Color::MagentaLight),
                separator(),
                check_debug->Render(),
                separator(),
                text("F0 Pitch Extraction Algorithm:") | bold,
                radio_f0->Render(),
                separator(),
                hbox({
                    vbox({text("Model Amplitude:") | bold, input_amp->Render() | border}) | flex,
                    vbox({text("FFT Size:") | bold, input_fft->Render() | border}) | flex,
                    vbox({text("AP Threshold:") | bold, input_ap->Render() | border}) | flex,
                }),
                separator(),
                btn_apply_settings->Render() | center,
                separator(),
                text("Status: " + settings_status) | bold | color(Color::Yellow),
            });
        } else {
            content_element = vbox({
                text(" About lessampler ") | bold | color(Color::CyanLight),
                separator(),
                text(" _                           _         ") | color(Color::Cyan),
                text("| |___ ___ ___ ___ _____ ___| |___ ___ ") | color(Color::Cyan),
                text("| | -_|_ -|_ -| .'|     | . | | -_|  _|") | color(Color::Cyan),
                text("|_|___|___|___|__,|_|_|_|  _|_|___|_|  ") | color(Color::Cyan),
                text(" Version: " PROJECT_GIT_HASH "       |_|            ") | color(Color::Cyan),
                separator(),
                text("• Singing Voice Synthesizer & Resampler for UTAU / OpenUtau") | bold,
                text("• Core Vocoder: WORLD (Harvest/Dio F0, CheapTrick, D4C Aperiodicity)") | dim,
                text("• Developer: YuzukiTsuru <GloomyGhost@GloomyGhost.com>") | dim,
                separator(),
                text("Navigation:") | bold,
                text("  [Tab] / [Shift+Tab]   Navigate through controls") | dim,
                text("  [Arrow Keys]          Select tabs / radio options") | dim,
                text("  [Enter]               Activate button / edit input") | dim,
                text("  [Esc]                 Exit TUI") | dim,
                separator(),
                btn_exit->Render() | center,
            });
        }

        return vbox({
            hbox({
                text(" 🎙️ LESSAMPLER CONTROL CENTER ") | bold | color(Color::White) | bgcolor(Color::BlueLight),
                filler(),
                text(" v" PROJECT_GIT_HASH " ") | dim | color(Color::GrayLight),
            }),
            separator(),
            hbox({
                menu_tabs->Render() | flex,
                btn_exit->Render(),
            }),
            separator(),
            content_element | flex | border,
        });
    });

    // Catch ESC key to exit easily
    auto main_with_event = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(main_with_event);
}
