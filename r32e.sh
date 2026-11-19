#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 900 > server.r32e.log 2>&1 &
SRV=$!
sleep 1
printf 'PRE-CONNECT %s\n' "$(date +%T)"
exec 3<>/dev/tcp/127.0.0.1/2600
printf 'CONNECTED  %s\n' "$(date +%T)"
cat <&3 | tee r32e_transcript.txt > /dev/null &
send() { printf 'SENT %s: %s\n' "$(date +%T)" "$1"; printf '%s\n' "$1" >&3; sleep 1.1; }
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
