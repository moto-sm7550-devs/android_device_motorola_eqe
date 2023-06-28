/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Not a Contribution
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2_0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2_0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <aidl/android/hardware/gnss/IGnss.h>
#include <hidl/LegacySupport.h>
#include "loc_cfg.h"
#include "loc_misc_utils.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include "Gnss.h"
#include <pthread.h>
#include <log_util.h>

extern "C" {
#include "vndfwk-detect.h"
}
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "android.hardware.gnss-aidl-impl-qti"

#ifdef ARCH_ARM_32
#define DEFAULT_HW_BINDER_MEM_SIZE 65536
#endif

using android::hardware::configureRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::joinRpcThreadpool;
using ::android::sp;

typedef int vendorEnhancedServiceMain(int /* argc */, char* /* argv */ []);
typedef void createQesdkHandle();

using GnssAidl = ::android::hardware::gnss::aidl::implementation::Gnss;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    ALOGI("%s, start Gnss HAL process", __FUNCTION__);

    std::shared_ptr<GnssAidl> gnssAidl = ndk::SharedRefBase::make<GnssAidl>();
    const std::string instance = std::string() + GnssAidl::descriptor + "/default";
    if (gnssAidl != nullptr) {
        binder_status_t status =
            AServiceManager_addService(gnssAidl->asBinder().get(), instance.c_str());
        if (STATUS_OK == status) {
            ALOGD("register IGnss AIDL service success");
        } else {
            ALOGE("Error while register IGnss AIDL service, status: %d", status);
        }
    }

    int vendorInfo = getVendorEnhancedInfo();
    // The magic number 2 points to
    // #define VND_ENHANCED_SYS_STATUS_BIT 0x02 in vndfwk-detect.c
    bool vendorEnhanced = ( vendorInfo & 2 );
    setVendorEnhanced(vendorEnhanced);


        // Loc AIDL service
#define VENDOR_AIDL_LIB "vendor.qti.gnss-service.so"
#define QESDK_SERVICE_LIB "liblocation_qesdk.so"
    void* libQesdkHandle = NULL;
    createQesdkHandle* qesdkMainMethod = (createQesdkHandle*)
        dlGetSymFromLib(libQesdkHandle, QESDK_SERVICE_LIB, "createLocationQesdk");
    if (NULL != qesdkMainMethod) {
        ALOGI("start Location QESDK service");
        (*qesdkMainMethod)();
    }

    void* libAidlHandle = NULL;
    vendorEnhancedServiceMain* aidlMainMethod = (vendorEnhancedServiceMain*)
        dlGetSymFromLib(libAidlHandle, VENDOR_AIDL_LIB, "main");
    if (NULL != aidlMainMethod) {
        ALOGI("start LocAidl service");
        (*aidlMainMethod)(0, NULL);
    }
    // Loc AIDL service end
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE;  // should not reach
}
