#!/bin/sh

grep '^Levl ' * | sort -n -t ' ' +1 | sed -e 's/:Levl/ /' | column -c 2 -t | more
