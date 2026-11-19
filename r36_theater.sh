#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 1500 > server.r36.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | grep -vE --line-buffered 'keeper says|hunter says|wanderer says|watches|crosses|walks across|falls dark|lightens|You hear someone|listens to a voice|drifts across|The sky turns|crouches|shakes the mist|stands still|turns his face|pulls her cloak|strings her bow|scans the treeline|watches the rain|banks|takes shelter|shuts|opens|sweeps|watches the low|settle' | tee r36_transcript.txt &
send() { printf '%s\n' "$1" >&3; sleep 1.1; }
send "Curtiss"
send "take lantern"
send "drop lantern"
send "drop lantern"
send "search pool"
send "search moon"
send "quit"
wait $SRV
