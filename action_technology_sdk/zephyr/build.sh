#!/bin/bash -e

ROOT=$PWD
NAME=$0
ARGV=$*

if [ -d zephyr ]; then
	cd zephyr
fi

source ./zephyr-env.sh
python ${NAME%.*}.py $ARGV

cd $ROOT
