#!/bin/sh
#
# File:   SetGovernors.sh
# Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
#

#Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit
fi

#Get num cores
numCores="$(nproc --all)"

#Check if userspace governor exists or we must use the performance governor.
userspace_avail=false
governors=($(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors))
desired="userspace"

if (printf '%s\n' "${governors[@]}" | grep -xq ${desired}); then
    userspace_avail=true
fi

#Turn each core on, set it be governed by userspace, or performance, as available
for ((core = 0; core < numCores; core++)); do
    #Turn core on
    echo 1 >/sys/devices/system/cpu/cpu${core}/online

    if [ "$userspace_avail" == true ]; then
        #Write userspace governor
        echo userspace >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor
    else
        #Write performance governor
        echo performance >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor

        #Set the core to run at the maximum frequency
        maxFreq=$(cat /sys/devices/system/cpu/cpu${core}/cpufreq/cpuinfo_max_freq)
        minFreq=$(cat /sys/devices/system/cpu/cpu${core}/cpufreq/cpuinfo_min_freq)
        echo ${maxFreq} >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_max_freq
        echo ${minFreq} >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_min_freq
    fi
done

#Disable turbo
echo 0 >/sys/devices/system/cpu/cpufreq/boost
