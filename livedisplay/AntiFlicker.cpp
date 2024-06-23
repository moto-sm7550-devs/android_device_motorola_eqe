/*
 * SPDX-FileCopyrightText: 2019-2024 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AntiFlicker.h"

#include <android-base/file.h>
#include <android-base/strings.h>

namespace {
    constexpr const char *kFileDc = "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/dsi_display_dc";
};  // anonymous namespace

using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_1 {
namespace implementation {

AntiFlicker::AntiFlicker() {
    if (!access(kFileDc, R_OK | W_OK)) {
        file_ = kFileDc;
        enabled_mode_ = 1;
    } else {
        file_ = nullptr;
    }
}

bool AntiFlicker::isSupported() {
    return file_ != nullptr;
}

// Methods from ::vendor::lineage::livedisplay::V2_1::IAntiFlicker follow.
Return<bool> AntiFlicker::isEnabled() {
    std::string tmp;
    int32_t contents = 0;

    if (ReadFileToString(file_, &tmp)) {
        contents = std::stoi(Trim(tmp));
    }

    return contents > 0;
}

Return<bool> AntiFlicker::setEnabled(bool enabled) {
    return WriteStringToFile(enabled ? std::to_string(enabled_mode_) : "0", file_, true);
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
