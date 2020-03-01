#!/bin/bash

g++ ./input.cpp  -o ./release/keymgr -std=c++11  -lasound
chmod +x ./keymgr
cp ./S99keymgr ./release/S99keymgr
cp ./start.sh ./release/start.sh
