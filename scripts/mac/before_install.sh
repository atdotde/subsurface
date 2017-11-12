#!/bin/bash

#
# maybe I don't need to do this if I use brew update? The documentation
# is not all that great
#
#rvm get stable --auto-dotfiles
#rvm install 2.3.0

# install a couple more homebrew components
if [ -d ${TRAVIS_BUILD_DIR}/Homebrew ] && [ -f ${TRAVIS_BUILD_DIR}/Homebrew/complete ] ; then
	echo "Homebrew with all our packages is in cache - overwriting /usr/local"
	sudo tar xJfC ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
elif [ -d ${TRAVIS_BUILD_DIR}/Homebrew ] && [ -f ${TRAVIS_BUILD_DIR}/Homebrew/updated ] ; then
	echo "updated Homebrew is in cache - overwriting /usr/local"
	echo "now get our dependencies brewed"
	sudo tar xJfC ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
	brew install hidapi libusb libxml2 libxslt libzip openssl pkg-config libgit2
	touch c${TRAVIS_BUILD_DIR}/Homebrew/complete
	tar cJf ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
else
	# get Homebrew updated and written to the cache
	brew update
	mkdir -p ${TRAVIS_BUILD_DIR}/Homebrew
	touch c${TRAVIS_BUILD_DIR}/Homebrew/updated
	tar cJf ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
fi

# prep things so we can build for Mac
# we have a custom built Qt some gives us just what we need, including QtWebKit

cd ${TRAVIS_BUILD_DIR}

mkdir -p Qt/5.9.1

echo "Get custom Qt build and unpack it"
wget -q http://subsurface-divelog.org/downloads/Qt-5.9.1-mac.tar.xz
tar -xJ -C Qt/5.9.1 -f Qt-5.9.1-mac.tar.xz

sudo mkdir -p /Users/hohndel
cd /Users/hohndel
sudo ln -s /Users/travis/build/Subsurface-divelog/subsurface/Qt/5.9.1 Qt5.9.1
