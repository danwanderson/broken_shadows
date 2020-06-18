#!/usr/bin/env bash
# Written by Furey.
# With additions from Tony.
# Set the port number.
PORT=4000
if [ "$1" != "" ]
then
	PORT="$1"
fi

SHADOWSDIR=/srv/shadows
# Change to area directory.
cd "${SHADOWSDIR}/area" || exit

# Set limits.
ulimit -c unlimited

if [ -e shutdown.txt ]
then
	rm -f shutdown.txt
fi

while true
do 
    if [ -e "${SHADOWSDIR}/area/core" ] 
	then
		DAY=$(date +%m%d%Y)
		HOUR=$(date +%H%M%S)
		DATE="${DAY}_${HOUR}"
        gdb -batch "${SHADOWSDIR}/src/shadows" "${SHADOWSDIR}/area/core" > $SHADOWSDIR/core/core.$DATE.txt
        mv "${SHADOWSDIR}/area/core" "${SHADOWSDIR}/core/core.$DATE"
		rm -f "${SHADOWSDIR}/area/core"
    fi

    # Run shadows.
    ../src/shadows "${PORT}" > /dev/null 2>&1

    # Restart, giving old connections a chance to die.
    if [ -e shutdown.txt ] 
	then
		rm -f shutdown.txt
		exit 0
    fi

    sleep 10
done
