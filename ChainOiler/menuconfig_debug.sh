#!/bin/bash
#
# debug config

if [ -f sdkconfig.debug ] 
  echo "copy debug config..."
  cp -f sdkconfig.debug sdkconfig
fi

pio run --target menuconfig --environment esp32s2-debug ;

if [ -f sdkconfig ] 
  echo "copy config to debug save..."
  cp -f sdkconfig sdkconfig.debug
fi

