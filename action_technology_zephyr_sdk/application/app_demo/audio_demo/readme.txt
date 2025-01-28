1、mic录音到文件：
开始录音：app audio_app record start /NOR:/record.pcm
停止录音：app audio_app record stop
录音超过30秒自动停止。如果有同名录音文件存在，将会被新录音文件替代。

2、播放录音文件：
开始播放：app audio_app play start /NOR:/record.pcm
停止播放：app audio_app play stop

3、mic->喇叭
开始：app audio_app micbypass start
停止：app audio_app micbypass stop

以上三个功能，同一时间只能运行某一个。
