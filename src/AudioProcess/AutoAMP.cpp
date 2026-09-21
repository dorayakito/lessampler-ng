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

#include "AutoAMP.h"
#include "Utils/LOG.h"

AutoAMP::AutoAMP(ShinePara shine, double *x) : shine(std::move(shine)) {
    this->x_length = this->shine.output_samples;
    this->x = x;
    this->x_out = new double[x_length];

    YALL_DEBUG_ << "The X_LENGTH is: " + std::to_string(x_length);

    GetMaxAMP();
    YALL_DEBUG_ << "Get Max AMP is: " + std::to_string(MaxAMP);

    SetDefaultValue();
    YALL_DEBUG_ << "The Default PCM is: " + std::to_string(sample_value);

    YALL_DEBUG_ << "Applying AutoAMP...";
    DiminishedConsonantFricative();

    YALL_DEBUG_ << "Limit maximum amplitude";
    LimitMaximumAmplitude();
}

AutoAMP::AutoAMP(double *x, int x_length, double amp_val) {
    this->x_length = x_length;
    this->x = x;
    x_out = new double[x_length];
    YALL_DEBUG_ << "The X_LENGTH is: " + std::to_string(x_length);
    GetMaxAMP();
    YALL_DEBUG_ << "Get Max AMP is: " + std::to_string(MaxAMP);
    DiminishedConsonantFricative(amp_val);
    LimitMaximumAmplitude();
}

double *AutoAMP::GetAMP() {
    return x_out;
}

void AutoAMP::GetMaxAMP() {
    MaxAMP = 0.0;
    for (int i = 0; i < x_length; ++i) {
        if (!std::isnan(x[i]) && !std::isinf(x[i])) {
            double abs_val = std::abs(x[i]);
            if (abs_val > MaxAMP) {
                MaxAMP = abs_val;
            }
        }
    }
    if (MaxAMP == 0.0) {
        YALL_WARN_ << "Max AMP is Zero.";
    }
}

void AutoAMP::SetDefaultValue() {
    sample_value = default_sample_value;
}

void AutoAMP::DiminishedConsonantFricative() {
    double target_gain = 0.95 * (shine.volumes > 0.0 ? shine.volumes : 1.0);
    double factor = (MaxAMP > 1e-6) ? (target_gain / MaxAMP) : 1.0;
    if (factor > 4.0) factor = 4.0;

    for (int i = 0; i < x_length; ++i) {
        if (std::isnan(x[i]) || std::isinf(x[i])) {
            x_out[i] = 0.0;
        } else {
            x_out[i] = x[i] * factor;
        }
    }
}

void AutoAMP::DiminishedConsonantFricative(double amp_volumes) {
    double target_gain = 0.95 * amp_volumes;
    double factor = (MaxAMP > 1e-6) ? (target_gain / MaxAMP) : 1.0;
    if (factor > 4.0) factor = 4.0;

    for (int i = 0; i < x_length; ++i) {
        if (std::isnan(x[i]) || std::isinf(x[i])) {
            x_out[i] = 0.0;
        } else {
            x_out[i] = x[i] * factor;
        }
    }
}

void AutoAMP::LimitMaximumAmplitude() {
    for (int i = 0; i < x_length; ++i) {
        if (x_out[i] > MaxValue) {
            x_out[i] = MaxValue;
        } else if (x_out[i] < MinValue) {
            x_out[i] = MinValue;
        }
    }
}
