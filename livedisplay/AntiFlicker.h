/*
 * SPDX-FileCopyrightText: 2019-2024 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vendor/lineage/livedisplay/2.1/IAntiFlicker.h>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_1 {
namespace implementation {

using ::android::hardware::Return;

class AntiFlicker : public IAntiFlicker {
  public:
    AntiFlicker();
    bool isSupported();

    // Methods from ::vendor::lineage::livedisplay::V2_1::IAntiFlicker follow.
    Return<bool> isEnabled() override;
    Return<bool> setEnabled(bool enabled) override;

  private:
    const char* file_;
    int32_t enabled_mode_;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
