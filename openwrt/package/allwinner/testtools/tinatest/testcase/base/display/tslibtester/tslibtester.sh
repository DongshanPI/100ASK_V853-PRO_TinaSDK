#!/bin/ash

ts_test &

sleep 8

killall -q -KILL ts_test

ttrue "Is the cursor on the screen moving normally?"
if [ $? = 0 ]
then
echo "GOOD, tp test success."
return 0
else
echo "ERROR, tp test failed."
return 1
fi
