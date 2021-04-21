#!/bin/bash
#
# debug config

if [ -f sdkconfig.release ] 
  echo "copy release config..."
  cp -f sdkconfig.release sdkconfig
fi

pio run --target menuconfig --environment esp32s2-release ;

if [ -f sdkconfig ] 
  echo "copy config to release save..."
  cp -f sdkconfig sdkconfig.release
fi

