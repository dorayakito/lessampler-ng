/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <utility>
#include <algorithm>

#include "Utils/exception.h"
#include "Utils/LOG.h"
#include "AudioProcess.h"
#include "StaticCast.h"

AudioProcess::AudioProcess(lessAudioModel audioModel, ShinePara shine) : audioModel(std::move(audioModel)), shine(std::move(shine)) {
    YALL_DEBUG_ << "Init TransAudioModel default data...";
    InitTransAudioModel();
    YALL_DEBUG_ << "Time Stretch & Pitch Transformation...";
    TimeStretch();
}

lessAudioModel AudioProcess::GetTransAudioModel() {
    return transAudioModel;
}

void AudioProcess::InitTransAudioModel() {
    transAudioModel = audioModel;
}

void AudioProcess::PicthEqualizing() {
    // Kept for backward compatibility if needed, but pitch transformation is done cleanly in TimeStretch
}

double AudioProcess::GetAvgFreq() const {
    double freq_avg = 0.0, timePercent, r, p[6], q, base_timePercent = 0;
    for (int i = 0; i < static_cast<int>(audioModel.f0.size()); ++i) {
        timePercent = audioModel.f0[i];
        if (timePercent < 1000.0 && timePercent > 55.0) {
            r = 1.0;
            for (int j = 0; j <= 5; ++j) {
                if (i > j) {
                    q = audioModel.f0[i - j - 1] - timePercent;
                    p[j] = timePercent / (timePercent + q * q);
                } else {
                    p[j] = 1 / (1 + timePercent);
                }
                r *= p[j];
            }
            freq_avg += timePercent * r;
            base_timePercent += r;
        }
    }
    if (base_timePercent > 0) freq_avg /= base_timePercent;
    return freq_avg;
}

void AudioProcess::TimeStretch() {
    YALL_DEBUG_ << "Allocate memory for target audio f0, sp, ap";

    if (shine.required_frame == 0)
        throw parameter_error("The target audio frame length is 0");

    transAudioModel.f0.resize(shine.required_frame, 0.0);
    transAudioModel.spectrogram.resize(shine.required_frame, std::vector<double>(audioModel.w_length, 0.0));
    transAudioModel.aperiodicity.resize(shine.required_frame, std::vector<double>(audioModel.w_length, 1.0));

    auto avg_freq = GetAvgFreq();
    YALL_DEBUG_ << "Source Audio AVG Frequency: " + std::to_string(avg_freq);

    int total_src_frames = static_cast<int>(audioModel.f0.size());
    if (total_src_frames == 0) {
        return;
    }

    double _sample_sp_trans_index, _sample_ap_trans_index, _out_sample_index, _in_sample_index;
    int _sp_trans_index, _ap_trans_index;

    for (int i = 0; i < shine.required_frame; ++i) {
        _out_sample_index = audioModel.frame_period * i;
        if (_out_sample_index < shine.base_length) {
            _in_sample_index = shine.offset + _out_sample_index * shine.velocity;
        } else {
            _in_sample_index = shine.offset + shine.first_half_fixed_part + (_out_sample_index - shine.base_length) * shine.stretch_length;
        }

        _sample_sp_trans_index = _in_sample_index / audioModel.frame_period;
        _sp_trans_index = static_cast<int>(floor(_sample_sp_trans_index));
        _sample_sp_trans_index -= _sp_trans_index;

        if (_sp_trans_index < 0) {
            _sp_trans_index = 0;
            _sample_sp_trans_index = 0.0;
        }
        if (_sp_trans_index >= total_src_frames) {
            _sp_trans_index = total_src_frames - 1;
            _sample_sp_trans_index = 0.0;
        }

        auto orig_f0 = audioModel.f0[_sp_trans_index];
        bool is_voiced = (orig_f0 > 0.0);

        if (is_voiced) {
            double temp_f0 = orig_f0;
            if (_sp_trans_index < total_src_frames - 1) {
                auto temp_f0_next = audioModel.f0[_sp_trans_index + 1];
                if (temp_f0_next > 0.0) {
                    temp_f0 = temp_f0 * (1.0 - _sample_sp_trans_index) + temp_f0_next * _sample_sp_trans_index;
                }
            }

            // Pitch bend calculation
            double pb = 0.0;
            if (shine.pitch_step > 0 && !shine.pitch_bend.empty()) {
                _sample_ap_trans_index = _out_sample_index * 0.001 * audioModel.fs / shine.pitch_step;
                _ap_trans_index = static_cast<int>(floor(_sample_ap_trans_index));
                _sample_ap_trans_index -= _ap_trans_index;

                if (_ap_trans_index < 0) {
                    _ap_trans_index = 0;
                    _sample_ap_trans_index = 0.0;
                }
                if (_ap_trans_index >= static_cast<int>(shine.pitch_bend.size()) - 1) {
                    _ap_trans_index = static_cast<int>(shine.pitch_bend.size()) - 2;
                    if (_ap_trans_index < 0) _ap_trans_index = 0;
                    _sample_ap_trans_index = 0.0;
                }

                if (_ap_trans_index + 1 < static_cast<int>(shine.pitch_bend.size())) {
                    pb = shine.pitch_bend[_ap_trans_index] * (1.0 - _sample_ap_trans_index) +
                         shine.pitch_bend[_ap_trans_index + 1] * _sample_ap_trans_index;
                }
            }

            // Target note pitch
            auto pitch_base = shine.scale_num * pow(2.0, pb / 1200.0);

            // Apply voice vibrato / pitch modulation cleanly
            if (avg_freq > 0.0 && shine.modulation != 0.0 && temp_f0 > 0.0) {
                transAudioModel.f0[i] = pitch_base * pow(temp_f0 / avg_freq, shine.modulation * 0.01);
            } else {
                transAudioModel.f0[i] = pitch_base;
            }
        } else {
            // Unvoiced frame: F0 must be 0 for natural consonant/noise synthesis
            transAudioModel.f0[i] = 0.0;
        }

        // Interpolate Spectrogram (Formants)
        for (int j = 0; j < audioModel.w_length; ++j) {
            if (_sp_trans_index < total_src_frames - 1) {
                transAudioModel.spectrogram[i][j] = audioModel.spectrogram[_sp_trans_index][j] * (1.0 - _sample_sp_trans_index) +
                                                    audioModel.spectrogram[_sp_trans_index + 1][j] * _sample_sp_trans_index;
            } else {
                transAudioModel.spectrogram[i][j] = audioModel.spectrogram[_sp_trans_index][j];
            }
        }

        // Interpolate Aperiodicity
        int ap_idx = _sp_trans_index;
        if (_sample_sp_trans_index > 0.5 && ap_idx < total_src_frames - 1) {
            ++ap_idx;
        }
        for (int j = 0; j < audioModel.w_length; ++j) {
            transAudioModel.aperiodicity[i][j] = audioModel.aperiodicity[ap_idx][j];
        }
    }
}

[[maybe_unused]] void AudioProcess::interp1(const double *x, const double *y, int x_length, const double *xi, int xi_length, double *yi) {
    auto *h = new double[x_length - 1];
    int *k = new int[xi_length];

    for (int i = 0; i < x_length - 1; ++i) {
        h[i] = x[i + 1] - x[i];
    }

    for (int i = 0; i < xi_length; ++i) {
        k[i] = 0;
    }

    histc(x, x_length, xi, xi_length, k);

    for (int i = 0; i < xi_length; ++i) {
        double s = (xi[i] - x[k[i] - 1]) / h[k[i] - 1];
        yi[i] = y[k[i] - 1] + s * (y[k[i]] - y[k[i] - 1]);
    }

    delete[] k;
    delete[] h;
}

[[maybe_unused]] void AudioProcess::histc(const double *x, int x_length, const double *edges, int edges_length, int *index) {
    int count = 1;

    int i = 0;
    for (; i < edges_length; ++i) {
        index[i] = 1;
        if (edges[i] >= x[0]) break;
    }
    for (; i < edges_length; ++i) {
        if (edges[i] < x[count]) {
            index[i] = count;
        } else {
            index[i--] = count++;
        }
        if (count == x_length) break;
    }
    count--;
    for (i++; i < edges_length; ++i) index[i] = count;
}
