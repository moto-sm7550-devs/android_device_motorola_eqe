#!/vendor/bin/sh

SCRIPT_NAME="copy_qc_cert.sh"
WV_SRC="/vendor/etc/qcom_widevine_licenses.pfm"
WV_DEST="/mnt/vendor/persist/data/pfm/licenses/qcom_widevine_licenses.pfm"
SECURE_PROP="ro.boot.secure_hardware"
WV_COPIED="/mnt/vendor/persist/data/pfm/licenses/.wv_copy_done"
GPS_COPIED="/mnt/vendor/persist/data/pfm/licenses/.gps_copy_done"
GPS_CERT="/mnt/vendor/persist/data/pfm/licenses/898-898-no-exp-2574099444.pfm"
WV_CERT_CHECKSUM="b5f9275aa997e4754b4131c6c07eb697"

debug()
{
    echo "Debug: $*"
}

notice()
{
    echo "Debug: $*"
    echo "$SCRIPT_NAME: $*" > /dev/kmsg
}


reinstall_wv_cert()
{
	notice "copy widevine to perist start:"
	cp $WV_SRC $WV_DEST
    if [ "$?" == "0" ]
    then
	    echo "2" > $WV_COPIED
	    notice "copy widevine to perist done"
    fi
}

reinstall_gps_cert()
{
    notice "start re-instsll gps cert"
    mv "$GPS_CERT".inst "$GPS_CERT"
    if [ "$?" == "0" ]
    then
        echo "2" > $GPS_COPIED
        notice "re-instsll gps cert done"
    fi
}

current_md5=`md5sum $WV_DEST.inst | cut -d" " -f1`
notice "current_md5:$current_md5"
notice "wv_cert_checksum:$WV_CERT_CHECKSUM"

if [ -f $WV_COPIED ]
then
    WV_COPIED_LABEL=`cat $WV_COPIED`
    if [[ "$WV_COPIED_LABEL" == "2" && "$current_md5" == "$WV_CERT_CHECKSUM" ]]
    then
        notice "widevine licenses is already re-installed!"
    else
        notice "remove old widevine licenses"
        rm /mnt/vendor/persist/data/pfm/licenses/.wv_copy_done
        rm /mnt/vendor/persist/data/pfm/licenses/qcom_widevine_licenses.pfm.inst
        reinstall_wv_cert
    fi
else
    reinstall_wv_cert
fi

GPS_COPIED_LABEL=`cat $GPS_COPIED`
if [[ -f "$GPS_CERT".inst && "$GPS_COPIED_LABEL" != "2" ]]
then
    reinstall_gps_cert
fi

fsync $WV_COPIED
fsync $WV_DEST
fsync $GPS_CERT
fsync $GPS_COPIED
