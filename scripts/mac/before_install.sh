#!/bin/bash

set -x

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

# for our build we need an updated Homebrew with a few more components
# installed. We keep this in a Travis cache to reduce build time
if [ ! -d ${TRAVIS_BUILD_DIR}/Homebrew ] ; then
	echo "Something is wrong with the cache, this directory should have been created. Giving up."
	exit 1
fi

ls -lh ${TRAVIS_BUILD_DIR}/Homebrew

if [ -f ${TRAVIS_BUILD_DIR}/Homebrew/complete ] ; then
	echo "Homebrew with all our packages is in cache - overwriting /usr/local"
	sudo tar xJfC ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
else
	echo "cache appears to be empty - let's first update homebrew"
	brew update
	mkdir -p ${TRAVIS_BUILD_DIR}/Homebrew
	touch ${TRAVIS_BUILD_DIR}/Homebrew/updated
	echo "updated Homebrew, now get our dependencies brewed"
	brew install xz hidapi libusb libxml2 libxslt libzip openssl pkg-config libgit2
	touch ${TRAVIS_BUILD_DIR}/Homebrew/complete
	tar cf - /usr/local | xz -v -z -0 --threads=0 > ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz
	echo "Homebrew cache setup"
	ls -lh ${TRAVIS_BUILD_DIR}/Homebrew
fi

# prep things so we can build for Mac
# we have a custom built Qt some gives us just what we need, including QtWebKit

cd ${TRAVIS_BUILD_DIR}

mkdir -p Qt/5.9.1

echo "Get custom Qt build and unpack it"
wget -q http://subsurface-divelog.org/downloads/Qt-5.9.1-mac.tar.xz
tar -xJ -C Qt/5.9.1 -f Qt-5.9.1-mac.tar.xz

sudo mkdir -p /Users/hohndel
sudo ln -s ${TRAVIS_BUILD_DIR}//Qt/5.9.1 /Users/hohndel/Qt5.9.1
ls -l /Users/hohndel
