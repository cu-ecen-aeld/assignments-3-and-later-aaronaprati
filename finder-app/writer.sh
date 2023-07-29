#!/bin/sh

filesdir=${1}

searchstr=${2}

fileCount=0

matchCount=0

if [ -z "$filesdir"  ]
then
	echo "You did not specify all the parameters"
	exit 1
elif [ -z "$searchstr" ] 
then
	echo "You did not specify all the parameters"
	exit 1
elif [ -d "$filesdir" ]
then
	fileCount=$(ls $filesdir | wc -l)
	#matchCount=$(find $filesdir -name $searchstr | wc -l)
	matchCount=$(grep -r "$searchstr" "$filesdir" | wc -l) 
	echo "The number of files are  $fileCount and the number of matching lines are $matchCount"
	exit 0 
else
	echo "Directory is not on the system"
	exit 1
fi
