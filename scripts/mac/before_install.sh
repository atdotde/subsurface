#!/bin/bash

set -x

# install a couple more homebrew components
brew install hidapi libusb libxml2 libxslt libzip openssl pka-config

# prep things so we can build for Mac
# we have a custom built Qt some gives us just what we need, including QtWebKit

cd ${TRAVIS_BUILD_DIR}

rm -rf Qt
mkdir -p Qt/5.9.1

echo "Get custom Qt build and unpack it"
wget -q http://subsurface-divelog.org/downloads/Qt-5.9.1-mac.tar.xz
tar -xJ -C Qt/5.9.1 -f Qt-5.9.1-mac.tar.xz
