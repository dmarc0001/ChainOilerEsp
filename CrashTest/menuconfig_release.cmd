@echo off

if exist sdkconfig.release ( 
echo.
echo kopiere release config...
copy /Y sdkconfig.release sdkconfig
)


c:\users\dmarc\.platformio\penv\Scripts\pio.exe run --target menuconfig --environment esp32s2-production

if exist sdkconfig (
echo.
echo kopiere zu release config...
copy /Y sdkconfig sdkconfig.release
)
