@echo off

if exist sdkconfig.debug ( 
echo.
echo kopiere debug config...
copy /Y sdkconfig.debug sdkconfig
)


c:\users\dmarc\.platformio\penv\Scripts\pio.exe run --target menuconfig --environment http-debug

if exist sdkconfig (
echo.
echo kopiere zu debug config...
copy /Y sdkconfig sdkconfig.debug
)
