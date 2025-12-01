# !/bin/bash
# Requirements BASH file to dynamically install all dependencies
# Dependencies: Build Tools, Valgrind, UTHash

sudo apt-get update
sudo apt-get install -y uthash-dev
sudo apt-get install -y build-essential valgrind