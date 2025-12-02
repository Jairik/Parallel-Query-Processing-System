# !/bin/bash

########################################################################
# Installs all required dependencies for building and running the QPE.
#     Dependencies: Build Tools, Valgrind, UTHash, OpenMPI
########################################################################

sudo apt-get update
sudo apt-get install -y uthash-dev
sudo apt-get install -y build-essential valgrind
sudo apt install openmpi-bin libopenmpi-dev