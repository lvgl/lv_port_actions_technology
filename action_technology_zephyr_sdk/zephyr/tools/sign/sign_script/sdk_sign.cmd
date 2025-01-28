@echo off

::--------------------------------------------------------
::-- 先签名， 再添加param 和nandid
::--------------------------------------------------------

:: 如果要签名，先执行这个
::del /S /Q sdk_out\*

python rsa_sign_for_sdk.py -i sign_in\zephyr.bin -k test_key\prikey2048.pem  -o sign_out\app.bin
python rsa_sign_for_sdk.py -i sign_in\recovery.bin -k test_key\prikey2048.pem -c test_key\public2048.pem -o sign_out\recovery.bin
python rsa_sign_for_sdk.py -i sign_in\mbrec.bin -k test_key\prikey2048.pem -c test_key\public2048.pem -b test_key\public2048.pem -o sign_out\mbrec.bin


