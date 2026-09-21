/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Synthesis.h"

#include <utility>
#include <algorithm>
#include "Utils/LOG.h"

#include <world/synthesis.h>

Synthesis::Synthesis(lessAudioModel audioModel, int x_length) : audioModel(std::move(audioModel)), x_length(x_length) {
    YALL_DEBUG_ << "Allocate Out Memory, Length: " + std::to_string(x_length);
    AllocateMemory();
    YALL_DEBUG_ << "Synthesis Audio...";
    SynthesisWav();
}

Synthesis::~Synthesis() {
    delete[] x;
}

double *Synthesis::GetWavData() {
    return x;
}

void Synthesis::AllocateMemory() {
    x = new double[x_length];
    for (int i = 0; i < x_length; ++i) {
        x[i] = 0.0;
    }
}

void Synthesis::SynthesisWav() const {
    if (audioModel.f0.empty() || x_length <= 0) {
        return;
    }

    int f0_length = static_cast<int>(audioModel.f0.size());
    auto f0 = new double[f0_length];
    std::copy(audioModel.f0.begin(), audioModel.f0.end(), f0);

    auto spectrogram = new double *[f0_length];
    auto aperiodicity = new double *[f0_length];
    for (int i = 0; i < f0_length; ++i) {
        spectrogram[i] = new double[audioModel.w_length];
        aperiodicity[i] = new double[audioModel.w_length];
        std::copy(audioModel.spectrogram[i].begin(), audioModel.spectrogram[i].end(), spectrogram[i]);
        std::copy(audioModel.aperiodicity[i].begin(), audioModel.aperiodicity[i].end(), aperiodicity[i]);
    }

    // High quality minimum-phase WORLD synthesis
    ::Synthesis(f0, f0_length,
                const_cast<const double * const *>(spectrogram),
                const_cast<const double * const *>(aperiodicity),
                audioModel.fft_size,
                audioModel.frame_period,
                audioModel.fs,
                x_length,
                x);

    delete[] f0;
    for (int i = 0; i < f0_length; ++i) {
        delete[] spectrogram[i];
        delete[] aperiodicity[i];
    }
    delete[] spectrogram;
    delete[] aperiodicity;
}
