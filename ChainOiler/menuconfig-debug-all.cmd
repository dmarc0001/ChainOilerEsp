@echo off


c:\users\dmarc\.platformio\penv\Scripts\pio.exe run --target menuconfig --environment esp32s2-debug-stripe

if exist sdkconfig.esp32s2-debug-stripe (
echo.
echo kopiere debug configs...
copy /Y sdkconfig.esp32s2-debug-stripe sdkconfig.esp32s2-debug-raw
copy /Y sdkconfig.esp32s2-debug-stripe sdkconfig.esp32s2-debug-pwm
)

