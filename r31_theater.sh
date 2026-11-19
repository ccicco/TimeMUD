#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 900 > server.r31.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | grep -vE --line-buffered 'The sky turns|You hear someone moving|Oren the wanderer walks across|crouches by the pool|shakes the mist|stands still in the storm|turns his face up' | tee r31_transcript.txt &
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
