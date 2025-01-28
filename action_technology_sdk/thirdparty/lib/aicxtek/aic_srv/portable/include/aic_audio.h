#ifndef __PORTABLE_INCLUDE_AUDIO_H__
#define __PORTABLE_INCLUDE_AUDIO_H__

/*
*********************************************************************
* Description: aicxtek 音频通道初始化
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_audio_init(void);

/*
*********************************************************************
* Description: aicxtek 音频通道关闭
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************************
*/
void aic_audio_close(void);

/*
*********************************************************************
* Description: aicxtek 音频通道打开
* Arguments  : None.
* Return     : 0        success
* Note(s)    : None.
*********************************************************************
*/
int aic_audio_open(void);


/*
*********************************************************************
* Description: aicxtek 判断音频通道运行中
* Arguments  : None.
* Return     : true     运行中
               false    没有运行
* Note(s)    : None.
*********************************************************************
*/
bool aic_audio_is_run(void);

#endif // #ifndef __PORTABLE_INCLUDE_AUDIO_H__

