#! /bin/sh

PHOTO_NUM=50
COUNT=1000
CYCLE=20
ERROR=1

i=0

while [ $i -ne $COUNT ]
do
        let i+=1
        echo " ************$i TEST CAMERA************"
        camerademo tinatest 0 /mnt/UDISK reopen
        if [ $? -eq $ERROR ]; then
                echo " ************$i TEST ERROR************"
                exit 1
        fi
	sleep 1
done

echo " ************$COUNT TEST CAMERA OK************"
camerademo tinatest 0 /mnt/UDISK ${PHOTO_NUM}
