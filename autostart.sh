until ./tcFarmControl; do
    echo "tcFarmControl crashed with exit code $?.  Respawning.." >&2
    sleep 1
done
