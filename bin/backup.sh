#!/bin/sh

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
DATE=`date +%m%d%Y_%H%M` 
ARCHIVENAME="${EXENAME}.${DATE}.tar" 
MAILUSER=dwa@rochester.rr.com

# Make sure the temp directory is readable and writable
if [ ! -r "${TEMPDIR}" -o ! -w "${TEMPDIR}" ]
then
	echo "Error! Cannot write temp file. Exiting..."
	exit 1
fi

# Create the archive dir, if it doesn't exist already
if [ ! -e "${ARCHIVEDIR}" ]
then
	mkdir "${ARCHIVEDIR}" 
	
	# Error creating?
	if [ $? -ne 0 ]
	then
		echo "Error creating archive directory."
		echo "Setting archive directory to ${HOME}"
		ARCHIVEDIR="${HOME}"
	fi
fi

# Change into the MUD directory
cd "${HOME}/${MUDDIR}" 

if [ $? -ne 0 ]
then
	echo "Error changing into ${HOME}/${MUDDIR}"
	echo "Exiting..."
	exit 1
fi

if [ ! -d "${AREADIR}" -o ! -d "${SRCDIR}" ]
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

cd "${HOME}"

# Create the tar archive
tar cf "${TEMPDIR}/${ARCHIVENAME}" "${MUDDIR}" 

# Compress the archive
gzip "${TEMPDIR}/${ARCHIVENAME}" 

# Move the archive to the ARCHIVEDIR
mv "${TEMPDIR}/${ARCHIVENAME}.gz" "${ARCHIVEDIR}" 

if [ $? -ne 0 ]
then
	mail -s "Backup failed" "${MAILUSER}" <<EOM
Backup of ${ARCHIVENAME}.gz failed.
Possible causes include lack of disk space or quota.
EOM
else
	mail -s "Broken Shadows backup successful" "${MAILUSER}" <<EOM
Backup of ${ARCHIVENAME}.gz succeeded.

List of files backed up:
`tar tzf "${ARCHIVEDIR}/${ARCHIVENAME}.gz"`
EOM
fi
 
cd "${SRCDIR}"
#make
#make clean
