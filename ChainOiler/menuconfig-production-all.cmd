@echo off


c:\users\dmarc\.platformio\penv\Scripts\pio.exe run --target menuconfig --environment esp32s2-production-stripe


if exist sdkconfig.esp32s2-production-stripe (
echo.
echo kopiere debug configs...
copy /Y sdkconfig.esp32s2-production-stripe sdkconfig.esp32s2-production-raw
copy /Y sdkconfig.esp32s2-production-stripe sdkconfig.esp32s2-production-pwm
)

