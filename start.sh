#!/bin/bash
rm http/graphs/*
ulimit -c unlimited
./build/tcFarmControl;
