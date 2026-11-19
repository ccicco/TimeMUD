#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 900 > server.r31b.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | grep -vE --line-buffered 'says, "|watches .* arrive|crosses from|walks across|falls dark|lightens|You hear someone|listens to a voice|drifts across|The sky turns|crouches by the pool|shakes the mist|stands still|turns his face|pulls her cloak|strings her bow|scans the treeline|watches the rain|banks the|takes shelter|shuts the|opens the|sweeps the|watches the low' | tee r31b_transcript.txt &
send() { printf '%s\n' "$1" >&3; sleep 1.1; }
send "Curtiss"
send "go s"
send "take lantern"
send "light lantern"
send "go n"
send "go e"
send "go e"
send "go e"
send "go e"
send "go e"
send "go n"
send "go n"
send "go n"
send "look moon"
send "look brass ring"
send "look ring"
send "look lantern"
send "look pool"
send "quit"
wait $SRV
