/* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_GnssAPIClient"
#define SINGLE_SHOT_MIN_TRACKING_INTERVAL_MSEC (590 * 60 * 60 * 1000) // 590 hours
#include <log_util.h>
#include <loc_cfg.h>

#include "GnssAPIClient.h"
#include <LocContext.h>
#include "LocationUtil.h"

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

static void convertGnssSvStatus(const GnssSvNotification& in,
        std::vector<IGnssCallback::GnssSvInfo>& out) {
    out.resize(in.count);
    for (size_t i = 0; i < in.count; i++) {
        gnss::aidl::implementation::convertGnssSvid(in.gnssSvs[i], out[i].svid);
        out[i].cN0Dbhz = in.gnssSvs[i].cN0Dbhz;
        out[i].elevationDegrees = in.gnssSvs[i].elevation;
        out[i].azimuthDegrees = in.gnssSvs[i].azimuth;
        out[i].carrierFrequencyHz = in.gnssSvs[i].carrierFrequencyHz;
        out[i].svFlag = static_cast<int>(IGnssCallback::GnssSvFlags::NONE);
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_HAS_EPHEMER_BIT)
            out[i].svFlag |= (int)IGnssCallback::GnssSvFlags::HAS_EPHEMERIS_DATA;
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_HAS_ALMANAC_BIT)
            out[i].svFlag |= (int)IGnssCallback::GnssSvFlags::HAS_ALMANAC_DATA;
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_USED_IN_FIX_BIT)
            out[i].svFlag |= (int)IGnssCallback::GnssSvFlags::USED_IN_FIX;
        if (in.gnssSvs[i].gnssSvOptionsMask & GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT)
            out[i].svFlag |= (int)IGnssCallback::GnssSvFlags::HAS_CARRIER_FREQUENCY;

        gnss::aidl::implementation::convertGnssConstellationType(in.gnssSvs[i].type,
                out[i].constellation);
        out[i].basebandCN0DbHz = in.gnssSvs[i].basebandCarrierToNoiseDbHz;
    }
}

GnssAPIClient::GnssAPIClient(const shared_ptr<IGnssCallback>& gpsCb) :
    LocationAPIClientBase(),
    mControlClient(new LocationAPIControlClient()),
    mTracking(false),
    mReportSpeOnly(true),
    mLocationCapabilitiesMask(0),
    mLocationCapabilitiesCached(false),
    mSvStatusEnabled(false),
    mNmeaEnabled(false),
    mGnssCbIface(gpsCb) {
    LOC_LOGd("]: (%p)", &gpsCb);
    initLocationOptions();
    loc_param_s_type izatConfParamTable[] = {
        {"ANDROID_REPORT_SPE_ONLY",      &mReportSpeOnly,       nullptr, 'n'},
    };
    UTIL_READ_CONF(LOC_PATH_IZAT_CONF, izatConfParamTable);
}

GnssAPIClient::~GnssAPIClient() {
    LOC_LOGd("]: ()");
    if (mControlClient) {
        delete mControlClient;
        mControlClient = nullptr;
    }
}

void GnssAPIClient::setFlpCallbacks() {
    LOC_LOGd("Going to set Flp Callbacks...");
    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    locationCallbacks.trackingCb = [this](const Location& location) {
        onTrackingCb(location);
    };
    locAPISetCallbacks(locationCallbacks);
}

void GnssAPIClient::setCallbacks() {
    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    /*-------------------|-------------    PVT received --------------------|
     | Technology used   | By trackingCb     | By engineLocationInfoCallback|
     |-------------------|-------------------|------------------------------|
     |Modem PE only      | SPE               | SPE                          |
     |-------------------|-------------------|----------------------------  |
     |Modem + HLOS Boeing| Aggregated        | SPE and Aggregated           |
     |----------------------------------------------------------------------|
     * By default always register with engineLocationsInfoCb, drop
     * aggreated PVTs(if received), so this call back only report SPE no
     * matter what the techonolgy is used;
     * When config is set to 0, register with trackingCb, this is the call back
     * which will report aggregated PVT to Android GNSS API*/
    if (0 == mReportSpeOnly) {
        locationCallbacks.trackingCb = [this](Location location) {
            onTrackingCb(location);
        };
    } else {
        locationCallbacks.engineLocationsInfoCb = nullptr;
        locationCallbacks.engineLocationsInfoCb = [this](uint32_t count,
                GnssLocationInfoNotification* engineLocationInfoNotification) {
            onEngineLocationsInfoCb(count, engineLocationInfoNotification);
        };
    }

    locationCallbacks.batchingCb = nullptr;
    locationCallbacks.geofenceBreachCb = nullptr;
    locationCallbacks.geofenceStatusCb = nullptr;
    locationCallbacks.gnssLocationInfoCb = nullptr;

    locationCallbacks.gnssSvCb = nullptr;
    if (mSvStatusEnabled) {
        locationCallbacks.gnssSvCb = [this](const GnssSvNotification& gnssSvNotification) {
            onGnssSvCb(gnssSvNotification);
        };
    }

    locationCallbacks.gnssNmeaCb = nullptr;
    if (mNmeaEnabled) {
        locationCallbacks.gnssNmeaCb = [this](GnssNmeaNotification gnssNmeaNotification) {
            onGnssNmeaCb(gnssNmeaNotification);
        };
    }

    locationCallbacks.gnssMeasurementsCb = nullptr;

    locAPISetCallbacks(locationCallbacks);
}

// for GpsInterface
void GnssAPIClient::gnssUpdateCallbacks(const shared_ptr<IGnssCallback>& gpsCb) {
    if (gpsCb != nullptr) {
        setCallbacks();
    }
}

void GnssAPIClient::gnssUpdateFlpCallbacks() {
    if (mGnssCbIface != nullptr) {
        setFlpCallbacks();
    }
}

void GnssAPIClient::initLocationOptions() {
    // set default LocationOptions.
    memset(&mTrackingOptions, 0, sizeof(TrackingOptions));
    mTrackingOptions.size = sizeof(TrackingOptions);
    mTrackingOptions.minInterval = 1000;
    mTrackingOptions.minDistance = 0;
    mTrackingOptions.mode = GNSS_SUPL_MODE_STANDALONE;
}

bool GnssAPIClient::gnssStart() {
    LOC_LOGd("]: ()");
    mMutex.lock();
    mTracking = true;
    mMutex.unlock();
    locAPIStartTracking(mTrackingOptions);
    return true;
}

bool GnssAPIClient::gnssStop() {
    LOC_LOGd("]: ()");
    mMutex.lock();
    mTracking = false;
    mMutex.unlock();
    locAPIStopTracking();
    return true;
}

void GnssAPIClient::configSvStatus(bool enable) {
    if (enable != mSvStatusEnabled) {
        mSvStatusEnabled = enable;
        setCallbacks();
    }
}
void GnssAPIClient::configNmea(bool enable) {
    if (enable != mNmeaEnabled) {
        mNmeaEnabled = enable;
        setCallbacks();
    }
}
bool GnssAPIClient::gnssSetPositionMode(IGnss::GnssPositionMode mode,
        IGnss::GnssPositionRecurrence recurrence, uint32_t minIntervalMs,
        uint32_t preferredAccuracyMeters, uint32_t preferredTimeMs,
        GnssPowerMode powerMode, uint32_t timeBetweenMeasurement)
{
    LOC_LOGd("]: (%d %d %d %d %d %d %d)",
            (int)mode, recurrence, minIntervalMs, preferredAccuracyMeters,
            preferredTimeMs, (int)powerMode, timeBetweenMeasurement);
    bool retVal = true;

    if (0 == minIntervalMs) {
        minIntervalMs = 1000;
    }

    memset(&mTrackingOptions, 0, sizeof(TrackingOptions));
    mTrackingOptions.size = sizeof(TrackingOptions);
    mTrackingOptions.minInterval = minIntervalMs;
    if (IGnss::GnssPositionMode::MS_ASSISTED == mode ||
            IGnss::GnssPositionRecurrence::RECURRENCE_SINGLE == recurrence) {
        // We set a very large interval to simulate SINGLE mode. Once we report a fix,
        // the caller should take the responsibility to stop the session.
        // For MSA, we always treat it as SINGLE mode.
        mTrackingOptions.minInterval = SINGLE_SHOT_MIN_TRACKING_INTERVAL_MSEC;
    }
    if (mode == IGnss::GnssPositionMode::STANDALONE)
        mTrackingOptions.mode = GNSS_SUPL_MODE_STANDALONE;
    else if (mode == IGnss::GnssPositionMode::MS_BASED)
        mTrackingOptions.mode = GNSS_SUPL_MODE_MSB;
    else if (mode ==  IGnss::GnssPositionMode::MS_ASSISTED)
        mTrackingOptions.mode = GNSS_SUPL_MODE_MSA;
    else {
        LOC_LOGd("]: invalid GnssPositionMode: %d", (int)mode);
        retVal = false;
    }
    if (GNSS_POWER_MODE_INVALID != powerMode) {
        mTrackingOptions.powerMode = powerMode;
        mTrackingOptions.tbm = timeBetweenMeasurement;
    }
    mTrackingOptions.locReqEngTypeMask = LOC_REQ_ENGINE_SPE_BIT;
    if (0 == mReportSpeOnly) {
        mTrackingOptions.locReqEngTypeMask = LOC_REQ_ENGINE_FUSED_BIT;
    }
    locAPIUpdateTrackingOptions(mTrackingOptions);
    return retVal;
}

void GnssAPIClient::gnssDeleteAidingData(IGnss::GnssAidingData aidingDataFlags)
{
    LOC_LOGd("]: (%02x)", aidingDataFlags);
    if (mControlClient == nullptr) {
        return;
    }
    GnssAidingData data;
    memset(&data, 0, sizeof (GnssAidingData));
    data.sv.svTypeMask = GNSS_AIDING_DATA_SV_TYPE_GPS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT |
        GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT |
        GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT |
        GNSS_AIDING_DATA_SV_TYPE_NAVIC_BIT;
    data.posEngineMask = STANDARD_POSITIONING_ENGINE;

    if (aidingDataFlags == IGnss::GnssAidingData::ALL) {
        data.deleteAll = true;
    } else {
        uint32_t delAidDataFlag = (uint32_t)aidingDataFlags;
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::EPHEMERIS) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_EPHEMERIS_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::ALMANAC) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_ALMANAC_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::POSITION) {
            data.common.mask |= GNSS_AIDING_DATA_COMMON_POSITION_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::TIME) {
            data.common.mask |= GNSS_AIDING_DATA_COMMON_TIME_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::IONO) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_IONOSPHERE_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::UTC) {
            data.common.mask |= GNSS_AIDING_DATA_COMMON_UTC_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::HEALTH) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_HEALTH_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::SVDIR) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_DIRECTION_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::SVSTEER) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_STEER_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::SADATA) {
            data.sv.svMask |= GNSS_AIDING_DATA_SV_SA_DATA_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::RTI) {
            data.common.mask |= GNSS_AIDING_DATA_COMMON_RTI_BIT;
        }
        if (delAidDataFlag & (uint32_t)IGnss::GnssAidingData::CELLDB_INFO) {
            data.common.mask |= GNSS_AIDING_DATA_COMMON_CELLDB_BIT;
        }
    }
    mControlClient->locAPIGnssDeleteAidingData(data);
}

void GnssAPIClient::requestCapabilities() {
    // only send capablities if it's already cached, otherwise the first time LocationAPI
    // is initialized, capabilities will be sent by LocationAPI
    // we need to send capabilities if setCallback was called, that is why we force
    // this by calling updateCapabilities with 2nd parameter being true
    if (mLocationCapabilitiesCached) {
        updateCapabilities(mLocationCapabilitiesMask, true);
    }
}

void GnssAPIClient::gnssEnable(LocationTechnologyType techType) {
    LOC_LOGd("]: (%0d)", techType);
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIEnable(techType);
}

void GnssAPIClient::gnssDisable() {
    LOC_LOGd("]: ()");
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIDisable();
}

void GnssAPIClient::gnssConfigurationUpdate(const GnssConfig& gnssConfig) {
    LOC_LOGd("]: (%02x)", gnssConfig.flags);
    if (mControlClient == nullptr) {
        return;
    }
    mControlClient->locAPIGnssUpdateConfig(gnssConfig);
}

// callbacks
void GnssAPIClient::onCapabilitiesCb(LocationCapabilitiesMask capabilitiesMask) {
    LOC_LOGd("mLocationCapabilitiesMask=%02x capabilitiesMask=%02x",
             mLocationCapabilitiesMask, capabilitiesMask);

    updateCapabilities(capabilitiesMask, false);
}

void GnssAPIClient::updateCapabilities(LocationCapabilitiesMask capabilitiesMask,
                                       bool forceSendCapabilities) {

    // we need to send capabilities if setCallback was called no matter what
    // (forceSendCapabilities is true)
    // but we need to NOT send capabilities if they are not changed

    if (!forceSendCapabilities) {
        if (capabilitiesMask == mLocationCapabilitiesMask) {
            LOC_LOGd("New capabilities are the same as existing ones, just return");
            return;
        }
    }
    mLocationCapabilitiesMask = capabilitiesMask;
    mLocationCapabilitiesCached = true;
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    uint32_t data = 0;
    if ((capabilitiesMask & LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT) ||
            (capabilitiesMask & LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT) ||
            (capabilitiesMask & LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT) ||
            (capabilitiesMask & LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT)) {
        data |= IGnssCallback::CAPABILITY_SCHEDULING;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_GEOFENCE_BIT) {
        data |= IGnssCallback::CAPABILITY_GEOFENCING;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT) {
        data |= IGnssCallback::CAPABILITY_MEASUREMENTS;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MSB_BIT) {
        data |= IGnssCallback::CAPABILITY_MSB;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MSA_BIT) {
        data |= IGnssCallback::CAPABILITY_MSA;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_AGPM_BIT) {
        data |= IGnssCallback::CAPABILITY_LOW_POWER_MODE;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT) {
        data |= IGnssCallback::CAPABILITY_SATELLITE_BLOCKLIST;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_MEASUREMENTS_CORRECTION_BIT) {
        data |= IGnssCallback::CAPABILITY_MEASUREMENT_CORRECTIONS_FOR_DRIVING;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_ANTENNA_INFO) {
        data |= IGnssCallback::CAPABILITY_ANTENNA_INFO;
    }
    if (capabilitiesMask & LOCATION_CAPABILITIES_QWES_SV_POLYNOMIAL_BIT) {
        data |= IGnssCallback::CAPABILITY_SATELLITE_PVT;
    }

    IGnssCallback::GnssSystemInfo gnssInfo = { .yearOfHw = 2015, "aidl-impl" };

    if (capabilitiesMask & LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT) {
        gnssInfo.yearOfHw++; // 2016
        if (capabilitiesMask & LOCATION_CAPABILITIES_DEBUG_DATA_BIT) {
            gnssInfo.yearOfHw++; // 2017
            if (capabilitiesMask & LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT ||
                capabilitiesMask & LOCATION_CAPABILITIES_AGPM_BIT) {
                gnssInfo.yearOfHw++; // 2018
                if (capabilitiesMask & LOCATION_CAPABILITIES_PRIVACY_BIT) {
                    gnssInfo.yearOfHw++; // 2019
                    if (capabilitiesMask & LOCATION_CAPABILITIES_CONFORMITY_INDEX_BIT) {
                        gnssInfo.yearOfHw += 3; // 2022
                    }
                }
            }
        }
    }
    LOC_LOGV("%s:%d] set_system_info_cb (%d)", __FUNCTION__, __LINE__, gnssInfo.yearOfHw);

    if (gnssCbIface != nullptr) {
        auto r = gnssCbIface->gnssSetCapabilitiesCb(data);
        if (!r.isOk()) {
            LOC_LOGe("Error from AIDL gnssSetCapabilitiesCb");
        }
        r = gnssCbIface->gnssSetSystemInfoCb(gnssInfo);
        if (!r.isOk()) {
            LOC_LOGe("] Error from gnssSetSystemInfoCb");
        }
    }
}

void GnssAPIClient::onTrackingCb(const Location& location) {
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    bool isTracking = mTracking;
    mMutex.unlock();

    LOC_LOGd("]: (flags: %02x isTracking: %d)", location.flags, isTracking);

    if (!isTracking) {
        return;
    }

    if (gnssCbIface != nullptr) {
        GnssLocation gnssLocation;
        convertGnssLocation(location, gnssLocation);
        auto r = gnssCbIface->gnssLocationCb(gnssLocation);
        if (!r.isOk()) {
            LOC_LOGe("%s] Error from gnssLocationCb",
                __func__);
        }
    } else {
        LOC_LOGw("] No GNSS Interface ready for gnssLocationCb ");
    }

}

void GnssAPIClient::onGnssSvCb(const GnssSvNotification& gnssSvNotification) {
    LOC_LOGd("]: (count: %u)", gnssSvNotification.count);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        std::vector<IGnssCallback::GnssSvInfo> svInfoList;
        convertGnssSvStatus(gnssSvNotification, svInfoList);
        auto r = gnssCbIface->gnssSvStatusCb(svInfoList);
        if (!r.isOk()) {
            LOC_LOGe("%s] Error from gnssSvStatusCb",
                __func__);
        }
    }
}

void GnssAPIClient::onGnssNmeaCb(GnssNmeaNotification gnssNmeaNotification) {
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (gnssCbIface != nullptr) {
        const std::string s(gnssNmeaNotification.nmea);
        std::stringstream ss(s);
        std::string each;
        while (std::getline(ss, each, '\n')) {
            each += '\n';
            std::string nmeaString;
            nmeaString.append(each.c_str(), each.length());
            if (gnssCbIface != nullptr) {
                auto r = gnssCbIface->gnssNmeaCb(
                        static_cast<long>(gnssNmeaNotification.timestamp), nmeaString);
                if (!r.isOk()) {
                    LOC_LOGe("%s] Error from gnssCbIface nmea=%s length=%u",
                             __func__, gnssNmeaNotification.nmea, gnssNmeaNotification.length);
                }
            }
        }
    }
}

void GnssAPIClient::onEngineLocationsInfoCb(uint32_t count,
            GnssLocationInfoNotification* engineLocationInfoNotification) {
    if (nullptr == engineLocationInfoNotification) {
        LOC_LOGe("engineLocationInfoNotification is nullptr");
        return;
    }
    GnssLocationInfoNotification* locPtr = nullptr;
    bool foundSPE = false;

    for (int i = 0; i < count; i++) {
        locPtr = engineLocationInfoNotification + i;
        if (nullptr == locPtr) return;
        LOC_LOGv("count %d, type %d", i, locPtr->locOutputEngType);
        if (LOC_OUTPUT_ENGINE_SPE == locPtr->locOutputEngType) {
            foundSPE = true;
            break;
        }
    }
    if (foundSPE) {
        onTrackingCb(locPtr->location);
    }
}
void GnssAPIClient::onStartTrackingCb(LocationError error) {
    LOC_LOGd("]: (%d)", error);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (error == LOCATION_ERROR_SUCCESS) {
        if (gnssCbIface != nullptr) {
            auto r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_ON);
            if (!r.isOk()) {
                LOC_LOGe("] Error from gnssStatusCb  ENGINE_ON");
            }
            r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_BEGIN);
            if (!r.isOk()) {
                LOC_LOGe("] Error from gnssStatusCb  SESSION_BEGIN");
            }
        }
    }
}

void GnssAPIClient::onStopTrackingCb(LocationError error) {
    LOC_LOGd("]: (%d)", error);
    mMutex.lock();
    auto gnssCbIface(mGnssCbIface);
    mMutex.unlock();

    if (error == LOCATION_ERROR_SUCCESS) {
        if (gnssCbIface != nullptr) {
            auto r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::SESSION_END);
            if (!r.isOk()) {
                LOC_LOGe(" Error from gnssStatusCb 2_0 SESSION_END");
            }
            r = gnssCbIface->gnssStatusCb(IGnssCallback::GnssStatusValue::ENGINE_OFF);
            if (!r.isOk()) {
                LOC_LOGe("] Error from gnssStatusCb 2_0 ENGINE_OFF");
            }
        }
    }
}

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
