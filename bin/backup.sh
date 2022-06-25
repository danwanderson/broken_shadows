#!/usr/bin/env bash

# B A C K U P . S H
# This script creates a backup of the MUD. It removes all core files
# (except those in the "core" directory)
# as well as object files (.o) and the executable before creating
# the archive. After the archive is created and compressed, it'll
# recompile the code and clean up the object files.

# Define some variables
# MUDDIR - location of the MUD
# TEMPDIR - place to store temp files
# AREADIR - directory area files are in
# SRCDIR - directory source files are in
# EXENAME - the name of the executable
# ARCHIVEDIR - where to store the archive once it's finished
# ARCHIVENAME - name of the archive - assigned automatically
MUDDIR="shadows"
TEMPDIR="/tmp"
AREADIR="${HOME}/${MUDDIR}/area"
SRCDIR="${HOME}/${MUDDIR}/src"
EXENAME="shadows"
ARCHIVEDIR="$HOME/archives"
DATE=$(date +%Y-%m-%d_%H-%M)
ARCHIVENAME="${EXENAME}.${DATE}.tar"
MAILUSER="someuser@example.com"

# Make sure the temp directory is readable and writable
if [ ! -r "${TEMPDIR}" ] || [ ! -w "${TEMPDIR}" ]
then
	echo "Error! Cannot write temp file. Exiting..."
	exit 1
fi

# Create the archive dir, if it doesn't exist already
if [ ! -e "${ARCHIVEDIR}" ]
then
 	if ! mkdir "${ARCHIVEDIR}"
	then
		echo "Error creating archive directory."
		echo "Setting archive directory to ${HOME}"
		ARCHIVEDIR="${HOME}"
	fi
fi

# Change into the MUD directory
if ! cd "${HOME}/${MUDDIR}"
then
	echo "Error changing into ${HOME}/${MUDDIR}"
	echo "Exiting..."
	exit 1
fi

if [ ! -d "${AREADIR}" ] || [ ! -d "${SRCDIR}" ]
then
	echo "Error. Either ${AREADIR} or ${SRCDIR} does not exist."
	echo "Exiting..."
	exit 2
fi

# Clean up core files, object files, and the executable
rm -f "${AREADIR}"/core
rm -f "${SRCDIR}"/*.o
#rm -f "${SRCDIR}/${EXENAME}"

# Really bad fix. According to the 'date' manpage, date is supposed
# to pad everything with 0s, but it doesn't do the hour, apparently.
#ARCHIVENAME=`echo "${ARCHIVENAME}" | sed -e 's/ /0/g'`

cd "${HOME}" || exit

# Create the tar archive
tar cf "${TEMPDIR}/${ARCHIVENAME}" "${MUDDIR}"

# Compress the archive
gzip "${TEMPDIR}/${ARCHIVENAME}"

# Move the archive to the ARCHIVEDIR
if ! mv "${TEMPDIR}/${ARCHIVENAME}.gz" "${ARCHIVEDIR}"
then
	mail -s "Backup failed" "${MAILUSER}" <<EOM
Backup of ${ARCHIVENAME}.gz failed.
Possible causes include lack of disk space or quota.
EOM
else
	mail -s "Broken Shadows backup successful" "${MAILUSER}" <<EOM
Backup of ${ARCHIVENAME}.gz succeeded.

List of files backed up:
$(tar tzf "${ARCHIVEDIR}/${ARCHIVENAME}.gz")
EOM
fi

#cd "${SRCDIR}" || exit
#make
#make clean
