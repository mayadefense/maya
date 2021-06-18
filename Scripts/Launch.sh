#!/bin/sh
#
# File:   Launch.sh
# Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
#

###################################Parse arguments##########################
usage() {
	echo
	echo "Usage: sudo -E --preserve-env=PATH env \"LD_LIBRARY_PATH=\$LD_LIBRARY_PATH\" bash $0"
	echo
	echo " -rd, --rundir    Run directory for Maya"
	echo " -ld, --logdir    directory for log files"
	echo " -o, --options   Options for Maya e.g., \"--mode Baseline\" \"--mode Sysid --idips CPUFreq\", \"--mode Mask GaussSine --ctldir ../../Controller/ --ctlfile mayaRobust\"" 
	echo " -t, --tag    tag for log files"
    echo " -a, --apps   apps to run"
  	echo " -h, --help   Display usage instructions"
	echo
}

if [ "$EUID" -ne 0 ] ; then
	usage	
	exit 1
fi

RUNDIR=
LOGDIR=
MAYA_OPTS=
TAG=
APPS=

while [ "$1" != "" ]; do
	case $1 in
		-rd | --rundir )   shift
			RUNDIR=$1
			;;
		-ld | --logdir )   shift
			LOGDIR=$1
			;;
	    -o | --options )   shift
			MAYA_OPTS=$1
			;;
		-t | --tag )   shift
			TAG=$1
            ;;
        -a | --apps )   shift
			APPS=$1
			;;
		-h | --help )   usage
			exit
			;;
		* ) # unsupported flags
			echo "Error: Unsupported flag $1" 
			usage
			exit 1
			;;
	esac
	shift
done

if [[ ! -d "$RUNDIR" ]]; then
	echo "Run directory $RUNDIR doesn't exist"
    usage
    exit 1
fi

if  [[ -z "$MAYA_OPTS" ]] ; then
    echo "Options for Maya weren't specified!"
	usage
	exit 1
fi

if [[ "$LOGDIR" ]] && [[ ! -d "$LOGDIR" ]]; then
	echo "You wanted to record the run in a log directory $LOGDIR but it doesn't exist."
    usage
    exit 1
elif [[ "$LOGDIR" ]] && [[ -z "$TAG" ]]; then
    echo "You wanted to record the run but haven't specified a tag for the run."
    usage
    exit 1
fi

NUM_CORES="$(nproc --all)"
BALLOON_NAME="Balloon"
BALLOON_CMD="./${BALLOON_NAME} ${NUM_CORES}"
MAYA_NAME="Maya"
MAYA_CMD="LD_LIBRARY_PATH=<PATH TO LIB64 directory that contains libstdc++.so (usually /usr/lib64/)>:\$LD_LIBRARY_PATH ./${MAYA_NAME} ${MAYA_OPTS}"

OUTFILE="/dev/null" #File where the output of the application run with Maya is stored
LOGFILE="/dev/null" #File where the output of Maya is stored
if [[ "$TAG" ]]; then
		OUTFILE=${LOGDIR}"/"${TAG}"_out.txt"
		LOGFILE=${LOGDIR}"/"${TAG}"_log.txt"
fi

function startall {
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

        maxFreq=$(cat /sys/devices/system/cpu/cpu${core}/cpufreq/cpuinfo_max_freq)

        if [ "$userspace_avail" == true ]; then
            #Write userspace governor
            echo userspace >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor
            
            #Set the core to run at the maximum frequency
            #echo ${maxFreq} >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_setspeed
        else
            #Write performance governor
            echo performance >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_governor

            #Set the core to run at the maximum frequency
            #echo ${maxFreq} >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_max_freq
            #echo ${maxFreq} >/sys/devices/system/cpu/cpu${core}/cpufreq/scaling_min_freq
        fi
    done

    #Disable turbo
    echo 0 >/sys/devices/system/cpu/cpufreq/boost

    #Launch the Balloon
    cd $RUNDIR
    echo -n "Launched balloon at "
    date +%s.%N
    eval $BALLOON_CMD &
    sleep 1

    #Launch Maya
    echo -n  "Launched controller at "
    date +%s.%N
    echo "Writing to $LOGFILE"
    eval $MAYA_CMD > $LOGFILE 2>&1 &
    sleep 1
}

function stopall {
    MAYA_PIDS=($(pgrep $MAYA_NAME$))
    if [ ${#MAYA_PIDS[@]} -ne 0 ]; then
    	echo -n "$MAYA_NAME is currently running and will be stopped. The PIDs are: "
        echo ${MAYA_PIDS[@]}
        for pid in "${MAYA_PIDS[@]}"; 	do
        	sudo kill -2 $pid
        done
    fi

    BALLOON_PIDS=($(pgrep $BALLOON_NAME$))
    if [ ${#BALLOON_PIDS[@]} -ne 0 ]; then
    	echo -n "$BALLOON_NAME is currently running and will be stopped. The PIDs are: "
        echo ${BALLOON_PIDS[@]}
        for pid in "${BALLOON_PIDS[@]}"; 	do
        	sudo kill $pid
        done
    fi
    
    #remove the balloon files   
    rm /dev/shm/powerBalloon.txt
    rm /dev/shm/powerBalloonMax.txt

    #Turn cores on
    for ((core=0;core<NUM_CORES;core++)); do
        echo 1 > /sys/devices/system/cpu/cpu${core}/online
    done

    #Turn turbo on
    echo 1 > /sys/devices/system/cpu/cpufreq/boost
}

########ctrl C handler###############
trap stopall SIGINT 

stopall
startall

if [[ "$APPS" == "yourapp" ]]; then
    #su <userid> -c "cmdline for your app" > $OUTFILE
	sleep 1
elif [[ "$APPS" == "empty" ]];then
    echo -n "Starting app empty at time " > $OUTFILE
    date +%s.%N >> $OUTFILE
    
    sleep 10 >> $OUTFILE
    
    echo -n "Completed app empty at time " >> $OUTFILE
    date +%s.%N >> $OUTFILE
else
    sleep 2
fi

stopall
