#!/usr/bin/env bash

# C L E A N _ P F I L E S . S H
# This script searches for and deletes all level 1 playerfiles

# All the variables. Change these as needed.
# SHADOWSDIR - where the MUD lies
# PLAYERDIR - where the pfiles lie
# TEMPDIR - where we will store the temp file
# TEMPFILE - the name of the temp file
# LOGDIR - where we'll store the log file
# LOGFILE - name of the log file
SHADOWSDIR="${HOME}/shadows"
PLAYERDIR="${SHADOWSDIR}/player"
TEMPDIR="/tmp"
TEMPFILE="${TEMPDIR}/clean_pfiles$$.tmp"
TEMPPFILE="${TEMPDIR}/pfiles.tmp"
LOGDIR="${HOME}/shadows"
LOGFILE="${LOGDIR}/removed_pfiles.log"

# Make sure the temp dir/file are readable/writable
if [ ! -w "$TEMPDIR" ] || [ ! -r "$TEMPDIR" ]
then
	echo "Error! Unable to create temp file. Exiting..."
	exit 1
fi

# Make sure the log dir/file are readable/writable
if [ ! -w "$LOGDIR" ] || [ ! -r "$LOGDIR" ]
then
	echo "Error! Unable to create log file. Exiting..."
	exit 1
fi

# Create the log file if it doesn't already exist and change the
# permissions to read/write (just in case)
touch "${LOGFILE}"
chmod 644 "${LOGFILE}"

# Change to the player directory
cd "${PLAYERDIR}" || exit

# Look for the level 1 player files and write the output to a 
# temp file
grep '^Levl 1$' ./* > "$TEMPFILE"

# The output of the grep command above is in the format
# <name>:Levl 1
# so we're going to strip the :Levl 1 from the list, leaving us
# with just the name of the pfile to remove
sed -e "s/:.*$//" "$TEMPFILE" > "$TEMPPFILE"

# Add some header info (such as the date the script was run)
# to the log file and some nice formatting
{
	echo ""
	date
	echo "----------------------------" 
} >> "$LOGFILE"

mv "$LOGFILE" "${LOGFILE}.old"

# Look through the list of pfiles that we've created and remove them,
# dumping the name of the pfile into the log file
while IFS= read -r NAME
do
	rm -f "$NAME"
	echo "$NAME" | tee -a "$LOGFILE"
done < "$TEMPPFILE"

# Clean out pfiles that haven't been modified in 4 months
cd "${PLAYERDIR}" || exit

find "${PLAYERDIR}" -mtime +120 -type f -printf %f\\n -exec rm -f {} \; | tee -a "${LOGFILE}"

cp "$LOGFILE" "${SHADOWSDIR}/bin/stats.removed"

cat "$LOGFILE" >> "${LOGFILE}.old"

mv "${LOGFILE}.old" "$LOGFILE"

echo "" >> "$LOGFILE"

# Remove the temp file
rm -f "$TEMPFILE"
rm -f "$TEMPPFILE"
