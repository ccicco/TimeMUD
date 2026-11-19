#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 4000 > server.r30b.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | while IFS= read -r l; do printf '%s %s\n' "$(date +%T)" "$l"; done | tee r30b_transcript.txt &
send() { printf 'SEND %s %s\n' "$(date +%T)" "$1" >&2; printf '%s\n' "$1" >&3; sleep 1.1; }
send "Curtiss"
send "look ring"
send "go s"
send "take lantern"
send "light lantern"
send "look ring"
send "go n"
send "go e"
send "go e"
send "go e"
send "go e"
send "go e"
send "go n"
send "go n"
send "go n"
send "look ring"
send "look lantern"
send "look pool"
send "quit"
wait $SRV
