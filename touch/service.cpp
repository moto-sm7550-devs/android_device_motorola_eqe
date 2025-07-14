/*
 * Copyright (C) 2020-2025 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "vendor.lineage.touch-service.eqe"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "HighTouchPollingRate.h"
#include "TouchscreenGesture.h"

using aidl::vendor::lineage::touch::HighTouchPollingRate;
using aidl::vendor::lineage::touch::TouchscreenGesture;

int main() {
    binder_status_t status = STATUS_OK;

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<HighTouchPollingRate> htpr = ndk::SharedRefBase::make<HighTouchPollingRate>();
    status = AServiceManager_addService(
            htpr->asBinder().get(), HighTouchPollingRate::makeServiceName("default").c_str());
    CHECK_EQ(status, STATUS_OK) << "Cannot register touchscreen glove HAL service.";

    std::shared_ptr<TouchscreenGesture> tg = ndk::SharedRefBase::make<TouchscreenGesture>();
    status = AServiceManager_addService(
            tg->asBinder().get(), TouchscreenGesture::makeServiceName("default").c_str());
    CHECK_EQ(status, STATUS_OK) << "Cannot register touchscreen gesture HAL service.";

    LOG(INFO) << "Touchscreen HAL service ready.";

    ABinderProcess_joinThreadPool();

    LOG(ERROR) << "Touchscreen HAL service failed to join thread pool.";
    return EXIT_FAILURE;  // should not reach
}
