#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 6000 > server.r38.log 2>&1 &
SRV=$!
sleep 1
printf 'CONNECT %s\n' "$(date +%T)"
exec 3<>/dev/tcp/127.0.0.1/2600
send() { printf '%s\n' "$1" >&3; sleep 1.1; }
send "Curtiss"
send "quit"
wait $SRV
printf 'CLOSED  %s\n' "$(date +%T)"
