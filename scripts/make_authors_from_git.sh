#!/bin/bash

AUTHORS="../AUTHORS.md"
if ! [ -f "$AUTHORS" ]
then
    AUTHORS="./AUTHORS.md"
    if ! [ -f "$AUTHORS" ]
    then
	echo "no authors file found, exiting"
	exit
    fi
fi

# order by number of commits
git log --format='%aN' | \
    sed 's/Francois/François/' | \
    sed 's/François/François/' | \
    sed 's/ubald/François Ubald Brien/' | \
    sed 's/nicolas/Nicolas Bouillot/' | \
    sed 's/Nina/Nicolas Bouillot/' | \
    sed 's/Jeremie Soria/Jérémie Soria/' | \
    sed 's/vlaurent/Valentin Laurent/' | \
    grep -v metalab | \
    grep -v 4d3d3d3 | \
    sort | \
    uniq -c | sort -bgr | \
    sed 's/\ *[0-9]*\ /\* /' > ${AUTHORS}

