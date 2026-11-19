#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 3600 > server.r42.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | tee r42_transcript.txt &
send() { printf '%s\n' "$1" >&3; sleep 1.1; }
send "Curtiss"
sleep 25
send "quit"
wait $SRV
