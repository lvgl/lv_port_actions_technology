#!/bin/bash -e

ROOT_DIR=$PWD
SUBMOD_ROOT='zephyr framework/bluetooth framework/media'

echo -e "\n--== git cleanup ==--\n"
west forall -c "git reset --hard"
west forall -c "git clean -d -f -x"

echo -e "\n--== git update ==--\n"
git fetch
git rebase
west update

echo -e "\n--== submodule update ==--\n"
for i in $SUBMOD_ROOT; do
	subdir=$ROOT_DIR/../${i}
	cd $subdir
	echo -e "\n--== $subdir update ==--\n\n"
	git submodule init
	git submodule update
done

cd $ROOT_DIR
