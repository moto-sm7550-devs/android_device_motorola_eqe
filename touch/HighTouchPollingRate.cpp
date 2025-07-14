/*
 * Copyright (C) 2022-2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "HighTouchPollingRateService"

#include "HighTouchPollingRate.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

namespace aidl {
namespace vendor {
namespace lineage {
namespace touch {

const std::string kInterpolationPath = "/sys/class/touchscreen/primary/interpolation";

ndk::ScopedAStatus HighTouchPollingRate::getEnabled(bool* _aidl_return) {
    std::string buf;
    if (!android::base::ReadFileToString(kInterpolationPath, &buf)) {
        LOG(ERROR) << "Failed to read " << kInterpolationPath;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    *_aidl_return = std::stoi(android::base::Trim(buf)) == 1;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HighTouchPollingRate::setEnabled(bool enabled) {
    if (!android::base::WriteStringToFile(std::to_string(enabled), kInterpolationPath)) {
        LOG(ERROR) << "Failed to write " << kInterpolationPath;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace touch
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
