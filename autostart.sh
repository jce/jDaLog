#!/bin/bash

./linkgraphs
sudo bash -c "echo 120000 > /proc/sys/net/ipv4/neigh/default/base_reachable_time_ms"

until ./tcFarmControl; do
    echo "tcFarmControl crashed with exit code $?.  Respawning.." >&2
    sleep 1
done
