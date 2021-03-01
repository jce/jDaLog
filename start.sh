#!/bin/bash
rm http/graphs/*
ulimit -c unlimited
./tcFarmControl;
