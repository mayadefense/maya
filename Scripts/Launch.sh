#!/bin/sh
#
# File:   Launch.sh
# Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
#

RUNDIR=""
numCores="$(nproc --all)"
BALLOON_CMD="./Balloon numCores"


#Launch the Balloon
echo -n "Launched Balloon at "
date +%s.%N
eval $BALLOON_CMD &
sleep 1

#Launch Maya