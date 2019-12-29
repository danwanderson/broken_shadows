#!/usr/bin/env bash

# S T A T S . S H
# Script to generate some player statistics by reading through the log file
# for the day and by looking through the player files

# Written by Rahl (Daniel Anderson) (c) 1999

# Set up some variables
OLDPLAYER=
NEWPLAYER=
CURRPLAYER=
REMPLAYER=
SHADOWSDIR="$HOME/shadows"
STATSFILE="${SHADOWSDIR}/bin/stats.dat"
REMOVEDFILE="${SHADOWSDIR}/bin/stats.removed"
PLAYERDIR="${SHADOWSDIR}/player"
LOGDIR="${SHADOWSDIR}/log"
CURDATE=$(date)

# Change into the player directory
cd "${PLAYERDIR}" || exit

# Grab the old statistics (basically, just the previous number of
# pfiles, created/updated when this script was last run)
# If this file doesn't exist, you'll get an error when you first
# run it. Run it again and it should be ok.
if [ -f "{$STATSFILE}" ]
then
	source "${STATSFILE}"
fi

# See if there's any info on the number of pfiles that were removed
# by clean_pfiles.sh
if [ -e "${REMOVEDFILE}" ]
then
	REMPLAYER=$(wc -l < "${REMOVEDFILE}")
	rm -f "${REMOVEDFILE}"
fi

# Zero everything if it's empty. This makes expr complain less
# and also makes the output look a little nicer
if [ -z "${REMPLAYER}" ]
then
	REMPLAYER=0
fi

if [ -z "${NEWPLAYER}" ]
then
	NEWPLAYER=0
fi

if [ -z "${OLDPLAYER}" ]
then
	OLDPLAYER=0
fi

# Get the current number of pfiles
CURRPLAYER=$(find . -type f | wc -l)

# The number of new players since the script was last run
NEWPLAYER=$(expr "${CURRPLAYER}" - "${OLDPLAYER}")

# Pull out the spaces cuz they're ugly
REMPLAYER=$(echo "${REMPLAYER}" | sed -e 's/ //g')
NEWPLAYER=$(echo "${NEWPLAYER}" | sed -e 's/ //g')
CURRPLAYER=$(echo "${CURRPLAYER}" | sed -e 's/ //g')
OLDPLAYER=$(echo "${OLDPLAYER}" | sed -e 's/ //g')

# Remove the old stats file
if [ -e ${STATSFILE} ]
then
	rm -f ${STATSFILE}
fi

# Write out the number of players there are now so we can use
# it for next time
echo "export OLDPLAYER=${CURRPLAYER}" > ${STATSFILE}

# Write out date, too, so we know when the script was last run
echo "export OLDDATE=\"`date`\"" >> ${STATSFILE}

# This should be today's log...
LOGNAME=`date +%m-%d-%Y.log`

# If the log file doesn't exist, print a warning, otherwise gater
# stats, such as logins, logouts, etc
if [ ! -e "${LOGDIR}/${LOGNAME}" ]
then
	echo "${LOGDIR}/${LOGNAME} does not exist. Some statistics will"
	echo "not be available."
else
	LOGINS=`grep "has connected" "${LOGDIR}/${LOGNAME}" | wc -l`
	NEWLOGINS=`grep "new player" "${LOGDIR}/${LOGNAME}" | wc -l`
	CONNECTIONS=`grep "Sock.sinaddr" "${LOGDIR}/${LOGNAME}" | wc -l`
	LOGOUTS=`grep "has quit" "${LOGDIR}/${LOGNAME}" | wc -l`
	LINKDEATHS=`grep "EOF encountered on read" "${LOGDIR}/${LOGNAME}" | wc -l`
	DEATHS=`grep "killed by" "${LOGDIR}/${LOGNAME}" | wc -l`
fi


# Zero anything that's empty
if [ -z "${LOGINS}" ]
then
	LOGINS=0
fi

if [ -z "${NEWLOGINS}" ]
then
	NEWLOGINS=0
fi

if [ -z "${CONNECTIONS}" ]
then
	CONNECTIONS=0
fi

if [ -z "${LOGOUTS}" ]
then
	LOGOUTS=0
fi

if [ -z "${LINKDEATHS}" ]
then
	LINKDEATHS=0
fi

if [ -z "${DEATHS}" ]
then
	DEATHS=0
fi

# Logins is the number of old logins plus new players
LOGINS=`expr ${LOGINS} + ${NEWLOGINS}`

# Strip the spaces
LOGINS=`echo "${LOGINS}" | sed -e 's/ //g'`
CONNECTIONS=`echo "${CONNECTIONS}" | sed -e 's/ //g'`
LOGOUTS=`echo "${LOGOUTS}" | sed -e 's/ //g'`
LINKDEATHS=`echo "${LINKDEATHS}" | sed -e 's/ //g'`
DEATHS=`echo "${DEATHS}" | sed -e 's/ //g'`
NEWLOGINS=`echo "${NEWLOGINS}" | sed -e 's/ //g'`

# Zero some variables for by-level statistics
UNDERTEN=0
ELEVENTOTWENTY=0
TWENTYTOTHIRTY=0
THIRTYTOFOURTY=0
FOURTYTOFIFTY=0
FIFTYTOSIXTY=0
SIXTYTOSEVENTY=0
SEVENTYTOEIGHTY=0
EIGHTYTONINETY=0
HERO=0
IMMORTAL=0

LEVEL=0

# Change these to match the number of levels for your MUD.
# On Shadows, level 1-91 are mortal, with 91s as heros,
# 92+ is immortal
while [ ${LEVEL} -le 102 ]
do
	CURLEVEL=`grep "^Levl ${LEVEL}$" ${PLAYERDIR}/* | wc -l`
	
	if [ ${LEVEL} -le 10 ]
	then
		UNDERTEN=`expr ${UNDERTEN} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 20 ] && [ ${LEVEL} -ge 11 ]
	then
		ELEVENTOTWENTY=`expr ${ELEVENTOTWENTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 30 ] && [ ${LEVEL} -ge 21 ]
	then
		TWENTYTOTHIRTY=`expr ${TWENTYTOTHIRTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 40 ] && [ ${LEVEL} -ge 31 ]
	then
		THIRTYTOFOURTY=`expr ${THIRTYTOFOURTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 50 ] && [ ${LEVEL} -ge 41 ]
	then
		FOURTYTOFIFTY=`expr ${FOURTYTOFIFTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 60 ] && [ ${LEVEL} -ge 51  ]
	then
		FIFTYTOSIXTY=`expr ${FIFTYTOSIXTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 70 ] && [ ${LEVEL} -ge 61 ]
	then
		SIXTYTOSEVENTY=`expr ${SIXTYTOSEVENTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 80 ] && [ ${LEVEL} -ge 71 ]
	then
		SEVENTYTOEIGHTY=`expr ${SEVENTYTOEIGHTY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 90 ] && [ ${LEVEL} -ge 81 ]
	then
		EIGHTYTONINETY=`expr ${EIGHTYTONINETY} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -eq 91 ]
	then
		HERO=`expr ${HERO} + ${CURLEVEL}`
	fi

	if [ ${LEVEL} -le 102 ] && [ ${LEVEL} -ge 92 ]
	then
		IMMORTAL=`expr ${IMMORTAL} + ${CURLEVEL}`
	fi

	LEVEL=`expr ${LEVEL} + 1`
done

# Insert race and class stats here, if desired


# Print out the results
echo ""
echo "Current time:         ${CURDATE}"
echo "Script last run time: ${OLDDATE}"
echo ""
echo "Stats since last player file cleaning:"
echo "--------------------------------------"
echo "Removed players:      ${REMPLAYER}"

echo ""
echo "Stats since this script was last run:"
echo "--------------------------------------"
echo "New players:          ${NEWPLAYER}"

echo ""
echo "Stats for today:"
echo "--------------------------------------"
echo "Old player count:     ${OLDPLAYER}"
echo "Current players:      ${CURRPLAYER}"
echo "Connections:          ${CONNECTIONS}"
echo "Logins:               ${LOGINS} (${NEWLOGINS} new)"
echo "Logouts:              ${LOGOUTS}"
echo "Linkdeaths:           ${LINKDEATHS}"
echo "Player deaths:        ${DEATHS}"

echo ""
echo "Statistics by level:"
echo "--------------------------------------"
echo "Under 10:             ${UNDERTEN}"
echo "11-20:                ${ELEVENTOTWENTY}"
echo "21-30:                ${TWENTYTOTHIRTY}"
echo "31-40:                ${THIRTYTOFOURTY}"
echo "41-50:                ${FOURTYTOFIFTY}"
echo "51-60:                ${FIFTYTOSIXTY}"
echo "61-70:                ${SIXTYTOSEVENTY}"
echo "71-80:                ${SEVENTYTOEIGHTY}"
echo "81-90:                ${EIGHTYTONINETY}"
echo "91:                   ${HERO}"
echo "92+:                  ${IMMORTAL}"
