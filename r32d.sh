#!/data/data/com.termux/files/usr/bin/bash
cd ~/TimeMUD
./build/timemud 1800 > server.r32d.log 2>&1 &
SRV=$!
sleep 1
exec 3<>/dev/tcp/127.0.0.1/2600
cat <&3 | tee r32d_transcript.txt > /dev/null &
printf 'Curtiss\n' >&3
sleep 3
printf 'say A\nsay B\nsay C\nsay D\nsay E\nsay F\nsay G\nsay H\nsay I\nquit\n' >&3
printf 'BATCH WROTE at %%s\n' "$(date +%%T)"
sleep 25
grep -n "RECV\|DOOR\|World time" server.r32d.log | head -30
wait $SRV
