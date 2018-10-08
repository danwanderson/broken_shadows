#!/bin/sh
# Written by Furey.
# With additions from Tony.
# Set the port number.
PORT=1945
if [ "$1" != "" ]
then
	PORT="$1"
fi

# Change to area directory.
cd $HOME/shadows/area

# Set limits.
if [ -e shutdown.txt ]
then
	rm -f shutdown.txt
fi

while true
do 
    if [ -e $HOME/shadows/area/core ] 
	then
		DAY=`date +%m%d%Y`
		HOUR=`date +%H%M%S`
		DATE="${DAY}_${HOUR}"
        gdb -batch $HOME/shadows/src/shadows $HOME/shadows/area/core | mail -s "Broken Shadows Automatic Core Trace" dwa@nodezero.com 
        mv $HOME/shadows/area/core $HOME/shadows/core/core.$DATE
		rm -f $HOME/shadows/area/core
    fi

    # Run shadows.
    ../src/shadows $PORT > /dev/null 2>&1

    # Restart, giving old connections a chance to die.
    if [ -e shutdown.txt ] 
	then
		rm -f shutdown.txt
		exit 0
    fi

    sleep 10
done
