#!/bin/sh
#
# File:   Launch.sh
# Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
#

RUNDIR=""
numCores="$(nproc --all)"
BALLOON_CMD="./Balloon numCores"
MAYA_CMD=""

#Launch the Balloon
cd ${RUNDIR}
echo -n "Launched Balloon at "
date +%s.%N
eval $BALLOON_CMD &
sleep 1

#Launch Maya
cd ${RUNDIR}
echo -n  "Launched controller at "
date +%s.%N
echo "Writing to $logFile"
eval $ctlCmd > $logFile 2>&1 &
sleep 1
}