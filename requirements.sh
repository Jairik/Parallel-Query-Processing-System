#!/bin/bash

########################################################################
# Installs all required dependencies for building and running the QPE.
#     Dependencies: Build Tools, Valgrind, UTHash, OpenMPI
# Supports: apt (Debian/Ubuntu), pacman (Arch), dnf (Fedora/RHEL),
#           zypper (openSUSE)
########################################################################

set -e

if command -v apt-get &> /dev/null; then
    sudo apt-get update
    sudo apt-get install -y build-essential valgrind uthash-dev openmpi-bin libopenmpi-dev
elif command -v pacman &> /dev/null; then
    sudo pacman -Syu --needed --noconfirm base-devel valgrind uthash openmpi
elif command -v dnf &> /dev/null; then
    sudo dnf install -y gcc gcc-c++ make valgrind uthash-devel openmpi openmpi-devel
    echo "Note: on Fedora/RHEL you may need to run 'module load mpi/openmpi-x86_64' before building."
elif command -v zypper &> /dev/null; then
    sudo zypper install -y gcc gcc-c++ make valgrind uthash-devel openmpi openmpi-devel
else
    echo "Error: no supported package manager found (apt, pacman, dnf, zypper)." >&2
    exit 1
fi
