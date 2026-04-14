#!/bin/bash
rm http/graphs/*
ulimit -c unlimited
./build/jDaLog
