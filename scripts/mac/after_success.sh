#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

cd ${TRAVIS_BUILD_DIR}
find . -name Subsurface.app -ls
zip Subsurface.app

echo "Submitting the folloing App for continuous build release:"
ls -lh Subsurface.app.zip

# get and run the upload script
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash ./upload.sh Subsurface.app.zip

