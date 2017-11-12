#!/bin/bash

#
# maybe I don't need to do this if I use brew update? The documentation
# is not all that great
#
#rvm get stable --auto-dotfiles
#rvm install 2.3.0

# install a couple more homebrew components
if [ ! -d ${TRAVIS_BUILD_DIR}/Homebrew ] ; then
	# trying to get Homebrew taken care of and hopefully get cached
	brew update
	brew install hidapi libusb libxml2 libxslt libzip openssl pkg-config libgit2
	mkdir -p ${TRAVIS_BUILD_DIR}/Homebrew
	tar cJf ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
else
	echo "Homebrew is in cache"
	find /usr/local
	sudo tar xJfC ${TRAVIS_BUILD_DIR}/Homebrew/local.tar.xz /usr/local
	find /usr/local
fi

# prep things so we can build for Mac
# we have a custom built Qt some gives us just what we need, including QtWebKit

cd ${TRAVIS_BUILD_DIR}

if [ ! -d Qt ] ; then
	mkdir -p Qt/5.9.1

	echo "Get custom Qt build and unpack it"
	wget -q http://subsurface-divelog.org/downloads/Qt-5.9.1-mac.tar.xz
	tar -xJ -C Qt/5.9.1 -f Qt-5.9.1-mac.tar.xz
else
	echo "Hoorray - caching worked"
fi

sudo mkdir -p /Users/hohndel
cd /Users/hohndel
sudo ln -s /Users/travis/build/Subsurface-divelog/subsurface/Qt/5.9.1 Qt5.9.1
