#!/bin/bash

MAC_PREFIX="C1:"
MAC_SUFFIX=`sudo hciconfig hci0 | grep "BD Address" | cut -d ":" -f 3-7 | cut -d " " -f 1`
sudo hciconfig hci0 up
sleep 1
sudo /home/bedis/bdaddr/bdaddr -i hci0 -r $MAC_PREFIX$MAC_SUFFIX
sudo hciconfig hci0 reset
sudo systemctl restart bluetooth.service
sleep 1
sudo hciconfig hci0 up

exit 0
