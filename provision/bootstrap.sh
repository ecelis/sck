#!/usr/bin/env bash

# Update OS packages
yum -y update
yum -y groupinstall "base"
yum -y groupinstall "Development Tools"
# Install required dependencies, it assumes a REHEL6 base and Development Tools
yum -y install python-devel
# Symlink the project shared folder in $HOME
ln -s /vagrant sck
# Build and install pjsip dependencies
cd sck
./build.sh

