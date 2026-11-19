#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 2000 > server.r32c.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | grep -vE --line-buffered 'keeper says|hunter says|wanderer says|watches|crosses|walks across|falls dark|lightens|You hear someone|listens to a voice|drifts across|The sky turns|crouches|shakes the mist|stands still|turns his face|pulls her cloak|strings her bow|scans the treeline|watches the rain|banks|takes shelter|shuts|opens|sweeps|watches the low|settle' | tee r32c_transcript.txt &
send() { printf 'SENT %s: %s\n' "$(date +%T)" "$1"; printf '%s\n' "$1" >&3; printf 'WROTE %s\n' "$(date +%T)"; sleep 1.1; }
send "Curtiss"
send "say A"
send "say B"
send "say C"
send "say D"
send "say E"
send "say F"
send "say G"
send "say H"
send "say I"
send "quit"
wait $SRV
