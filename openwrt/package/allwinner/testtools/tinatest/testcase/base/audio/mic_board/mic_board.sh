#!/bin/sh

keypath="/base/audio/mic_board"

capture_pcm_device=$(mjson_fetch "${keypath}/capture_pcm_device")
capture_channels=$(mjson_fetch "${keypath}/capture_channels")
capture_format=$(mjson_fetch "${keypath}/capture_format")
capture_rate=$(mjson_fetch "${keypath}/capture_rate")
capture_duration_sec=$(mjson_fetch "${keypath}/capture_duration_sec")
record_file=$(mjson_fetch "${keypath}/record_file")
remove_record_file=$(mjson_fetch "${keypath}/remove_record_file")
led_count=$(mjson_fetch "${keypath}/led_count")
led_color=$(mjson_fetch "${keypath}/led_color")
led_brightness=$(mjson_fetch "${keypath}/led_brightness")

[ ! -d "$(dirname ${record_file})" ] && mkdir -p "$(dirname ${record_file})"

ttips "Start recording (in $capture_duration_sec seconds)"

arecord -D "$capture_pcm_device" -c "$capture_channels" -f "$capture_format" \
    -r "$capture_rate" -d "$capture_duration_sec" "$record_file"
if [ $? -ne "0" ]; then
    ttips "Error occurred when recording"
    exit 1
fi

frame_bytes=$(expr 2 \* $capture_channels)
frame_total=$(expr $frame_bytes \* $capture_rate \* $capture_duration_sec)
wav_len_expect=$(expr $frame_total + 44)
wav_len=`ls -l $record_file | awk '{print $5}'`

if [ $wav_len_expect != $wav_len ]; then
    ttips "wav len error, $wav_len_expect != $wav_len"
    exit 1
fi

test_frames=128
test_frames_bytes=$(expr $test_frames \* $frame_bytes)
test_count=0
offset=44
while true;
do
    offset=$(($offset+$test_count))
    hex=`hexdump -s $offset -n 1 -e '1/1 "0x%02x""\n"' $record_file`
    if [ $hex != "0x00" ]; then
        break;
    fi
    test_count=$(($test_count+1))
    if [ $test_count = $test_frames_bytes ]; then
        ttips "$test_frames frames is silence data, sth error"
        exit 1
    fi
done

ttips "Record Test pass."

[ "x$remove_record_file" == "xtrue" ] && rm -f "$record_file"

# light on leds
count=0
trigger=none
while true;
do
    if [ $count -ge $led_count ]; then
        break;
    fi
    node="/sys/class/leds/sunxi_led$count${led_color:0:1}"
    count=$(($count+1))
    echo $trigger > $node/trigger
    echo $led_brightness > $node/brightness
done

ttrue "All led($led_color) lights on?"
if [ $? = 0 ]
then
    exit 0
else
    exit 1
fi
