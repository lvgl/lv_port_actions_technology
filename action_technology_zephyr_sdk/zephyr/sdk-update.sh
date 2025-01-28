#!/bin/bash
PRJ_VER=$1
PREFIX=${PRJ_VER:0:3}
VER_REF=`git show-ref refs/heads/$PRJ_VER`

if [ "$PRJ_VER" == "" ]; then
    echo "Usage: ./sdk-update.sh [branch/tag]"
    exit
fi

echo -e "\n--== git checkout $PRJ_VER ==--\n"

source ./zephyr-env.sh

git checkout .
git fetch

if [ "$PREFIX" == "TAG" ]; then
	if [ "$VER_REF" == "" ]; then
		git checkout -b $PRJ_VER tags/$PRJ_VER
	else
		git checkout $PRJ_VER
	fi
	west config manifest.file tag.yml
else
	if [ "$VER_REF" == "" ]; then
		git checkout -b $PRJ_VER remotes/origin/$PRJ_VER
	else
		git checkout $PRJ_VER
		git rebase remotes/origin/$PRJ_VER
	fi
	west config manifest.file west.yml
fi

./sdk-reset.sh
