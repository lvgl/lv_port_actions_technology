/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bluetooth tws observer interface
 */

#include "bluetooth_tws_observer.h"
#include "utils/acts_ringbuf.h"

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "bt_tws"

/*
1、如果两边都处于等待播放状态，设备收到对方的splay后，
   若收到的对方的起始播放时钟小于当前bt时钟，则做出错处理
   包号相同，则用时间较晚的中断时钟作为开始播放时间，重新设置tws中断时间
   若包号不同，则以包号大的作为起始播放包号和时钟，设置为PLAYER_TWS_USE_LOCAL_SPLAY模式，丢弃包号小的码流包
2、如果当前设备已经开始播放，收到到对方的splay包，
   则从当前缓存的码流包中，挑选一个比较靠后的包，计算播放时间，给对方发一个PLAYER_TWS_USE_LOCAL_SPLAY
   若码流缓存信息不足，则向对方发送PLAYER_TWS_NEED_NEW_SPLAY，对方收到消息后，重新发送splay过来
*/

//tws中断启动最小间隔,us
#define TWS_MIN_INTERVAL     (20000)

/* splay_flag的说明
 * 1: 本端处于PLAYING状态, 暂时只支持对端处于WAIT PLAY状态, 希望对端重新发一次splay包过来, 因为本端缓存的包里面没有对端的包号
 * 2: 本端处于PLAYING状态, 希望对端以本端的包号和时间来播放
 * 3: 本端处于WAIT PLAY状态, 若对端不是PLAYING, 希望对端考虑是否接受本端的包号和时间, 若对端是PLAYING, 希望对端提供可以播放的包号和时间
 */
#define PLAYER_TWS_NEED_NEW_SPLAY    1
#define PLAYER_TWS_USE_LOCAL_SPLAY   2
#define PLAYER_TWS_USE_START_SPLAY   3

#define PLAYER_TWS_SYNCITEM_MAXNUM      10      /* TWS同步信息包最大缓存个数 */
#define PLAYER_TWS_ERRCOUNT_MAXNUM      10      /* TWS失去同步状态的超时时间 */
#define PLAYER_TWS_DATA_MAXNUM      5      /* 暂存从对端接收到的消息个数 */
#define PLAYER_TWS_MSG_MAXNUM      5      /* 暂存从中间件api接收到的消息个数 */
#define BT_TWS_TIMER_INTERVAL     (3)


typedef enum
{
    TWS_STARTPLAY_PKT = 1,       /* 启动播放包 */
    TWS_STARTSTOP_PKT,       /* 停止播放包 */
    TWS_SYNCINFO_PKT,    /* 同步信息包 */
    TWS_PLAYRATE_PKT,        /* 速度调节包 */
    TWS_INTTIME_PKT,         /* 中断时间包 */
    TWS_BTTIME_PKT,          /* 蓝牙时间包 */
} player_tws_type_e;

/* TWS数据包, 需和tws_sync_cmd_t保持一致
 */
typedef struct
{
    uint8_t group;  /* 参考 player_tws_group_e */
    uint8_t type;   /* 参考 player_tws_type_e */
    uint8_t reserve;
    uint8_t payload_len;
    uint8_t payload_data[16]; /* buf需要4字节对齐, 避免直接赋值导致死机 */
} player_tws_pkt_t;

typedef struct
{
    uint8_t scene; /* 播放场景, 避免不同场景的数据包误判, player_id_e */
    uint8_t splay_flag; /* 本端发送SPLAY的时候告知对端的操作类型, PLAYER_TWS_NEED_NEW_SPLAY... */
    uint16_t pkt_num; /* 开始播放的首包号 */
    uint64_t splay_time; /* 开始播放的时钟 us */
    uint32_t first_clk; /* 本端接收到第一个数据包时的时钟 */
} player_tws_startplay_pkt_t;

typedef struct
{
    uint8_t scene; /* 播放场景, 避免不同场景的数据包误判 */
    uint8_t reserve[3];
    uint64_t startstop_time; /* 开始播放的时钟 us */
} player_tws_startstop_pkt_t;

typedef struct
{
    uint8_t   scene;
    uint16_t  pkt_num; /* 表示播放的数据包号 */
    uint8_t   reserve;
    uint64_t  bttime_us; /* 表示播放对应包号时对应的蓝牙时钟 us */
} player_tws_syncinfo_pkt_t;

typedef struct
{
    uint8_t  scene;
    uint8_t  aps_level;
    uint8_t  reserve[2];
    uint64_t inttime;  /* 中断时间 us */
} player_tws_playrate_pkt_t;

typedef enum
{
    PLAYER_TWS_STATUS_INIT,     /* 执行init后的状态 */
    PLAYER_TWS_STATUS_WAIT,     /* 执行wait play后的状态 */
    PLAYER_TWS_STATUS_DEAL_WAIT,/* 执行deal start play后的状态 */
    PLAYER_TWS_STATUS_PLAY,     /* 真正播放数据的状态 */
} player_tws_status_e;

typedef struct
{
    uint16_t pkt_num; /* 包号 */
    uint16_t pkt_len; /* 包的数据长度 */
    uint16_t samples; /* 样点数 */
    uint64_t pkt_bttime_us; /* 蓝牙时钟 us */
} player_tws_pinfo_unit_t;

/* TWS消息
 */
typedef struct
{
    uint8_t type;   /* 参考 player_tws_type_e */
    uint8_t data[15];
} __attribute__((packed)) player_tws_msg_t;

typedef struct {
    bt_tws_observer_t tws_observer;
    media_observer_t *media_observer;
    struct k_timer timer;

    uint8_t format;
    uint16_t sample_rate;  //hz
    uint16_t first_pktnum;
    uint32_t first_clk;

    //以下信息会被中断函数访问
    uint8_t tws_status;
    int8_t aps_level_pending;   /* 即将要设置的aps等级 */
    uint64_t aps_inttime;

    //以下信息会被多线程访问
    struct acts_ringbuf tws_data_buffer;
    player_tws_pkt_t tws_data[PLAYER_TWS_DATA_MAXNUM];

    uint8_t tws_role;
    uint8_t last_sinfo_flag;
    uint8_t first_pkt_flag;
    uint8_t sync_error_count; /* 同步错误计数 */
    int8_t adjust_shake_flag;   /* 防抖，master和slave前后两个包的时间差一致 */
    uint64_t splay_local_time; /* 近端开始播放的时间 */
    uint64_t forbit_aps_time; /* 在此时间前禁止调节水位aps */
    
    struct acts_ringbuf sinfo_master_buffer;
    struct acts_ringbuf sinfo_slave_buffer;
    struct acts_ringbuf msg_buffer;
    player_tws_syncinfo_pkt_t sinfo_master[PLAYER_TWS_SYNCITEM_MAXNUM];
    player_tws_syncinfo_pkt_t sinfo_slave[PLAYER_TWS_SYNCITEM_MAXNUM];
    player_tws_msg_t msg_data[PLAYER_TWS_MSG_MAXNUM];

    /* 包信息单元
     */
    uint8_t pinfo_unit_idx;
    uint8_t save_pinfo_count; /* 包信息保存计数 */
    player_tws_pinfo_unit_t pinfo_unit[2];

    uint16_t BM_TWS_WPlay_Mintime;
    uint16_t BM_TWS_WPlay_Maxtime;
    uint16_t BM_TWS_Sync_interval;
} bt_tws_observer_inner_t;

static bt_tws_observer_inner_t observer_inner;


/*----------------------------------------------------------------------------------------------------*/
//callback from irq
static void _tws_irq_handle(uint64_t bt_clk_us)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    
    if(handle->tws_status == PLAYER_TWS_STATUS_DEAL_WAIT)
    {
        handle->tws_status = PLAYER_TWS_STATUS_PLAY;
        handle->media_observer->start_playback(handle->media_observer->media_handle);
        printf("[ATW]start playback\n");
        return;
    }

    /* 调节速度中断(TWS播放时才有效)
     */
    if((handle->aps_level_pending >= 0)
        && (handle->aps_inttime == bt_clk_us)
        && (handle->tws_status == PLAYER_TWS_STATUS_PLAY))
    {
        handle->media_observer->set_base_aps_level(handle->media_observer->media_handle, handle->aps_level_pending);
        printf("[ATW]aps int:%d\n", handle->aps_level_pending);
        handle->aps_level_pending = -1;
    }
}

//callback from normal thread
static void _tws_recv_pkt_handle(void *buf, int32_t size)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_pkt_t *pkt = (player_tws_pkt_t*)buf;

    if(handle->tws_status == PLAYER_TWS_STATUS_INIT)
        return;
    if(size != sizeof(player_tws_pkt_t))
    {
        SYS_LOG_ERR("size: %d invalid, normal size: %d", size, sizeof(player_tws_pkt_t));
        return;
    }

    if(acts_ringbuf_space(&handle->tws_data_buffer) < sizeof(player_tws_pkt_t))
    {
        SYS_LOG_ERR("tws data buffer full");
        return;
    }
    
    acts_ringbuf_put(&handle->tws_data_buffer, buf, size);
}

/*----------------------------------------------------------------------------------------------------*/
static int32_t _tws_send_start_play(bt_tws_observer_inner_t *handle, player_tws_startplay_pkt_t *startplay)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_pkt_t pkt;
    int32_t ret;

    SYS_LOG_INF("set SPlay, pktnum: %d, time: %lld, fclk: %d, flag: %d",
        startplay->pkt_num, startplay->splay_time, startplay->first_clk, startplay->splay_flag);

    startplay->scene = handle->format;
    pkt.group = 0;
    pkt.type = TWS_STARTPLAY_PKT;
    pkt.payload_len = sizeof(player_tws_startplay_pkt_t);
    memcpy(pkt.payload_data, startplay, sizeof(player_tws_startplay_pkt_t));
    
    ret = observer->send_pkt(&pkt, sizeof(pkt));
    if(ret != sizeof(pkt))
        return -1;
    return 0;
}

/* 返回0: 表示数据包没有发送出去
 */
static int32_t _tws_send_start_stop(bt_tws_observer_inner_t *handle, player_tws_startstop_pkt_t *startstop)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_pkt_t pkt;
    int32_t ret;

    SYS_LOG_INF("set SStop: %lld", startstop->startstop_time);

    startstop->scene = handle->format;
    pkt.group = 0;
    pkt.type = TWS_STARTSTOP_PKT;
    pkt.payload_len = sizeof(player_tws_startstop_pkt_t);
    memcpy(pkt.payload_data, startstop, sizeof(player_tws_startstop_pkt_t));

    ret = observer->send_pkt(&pkt, sizeof(pkt));
    if(ret != sizeof(pkt))
        return -1;
    return 0;
}

/* 返回0: 表示数据包没有发送出去
 */
static int32_t _tws_send_sync_info(bt_tws_observer_inner_t *handle, player_tws_syncinfo_pkt_t *syncinfo)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_pkt_t pkt;
    int32_t ret;

    SYS_LOG_INF("set SInfo, pktnum: %d, time: %lld", syncinfo->pkt_num, syncinfo->bttime_us);

    syncinfo->scene = handle->format;
    pkt.group = 0;
    pkt.type = TWS_SYNCINFO_PKT;
    pkt.payload_len = sizeof(player_tws_syncinfo_pkt_t);
    memcpy(pkt.payload_data, syncinfo, sizeof(player_tws_syncinfo_pkt_t));

    ret = observer->send_pkt(&pkt, sizeof(pkt));
    if(ret != sizeof(pkt))
        return -1;
    return 0;
}

/* 返回0: 表示数据包没有发送出去
 */
static int32_t _tws_send_play_rate(bt_tws_observer_inner_t *handle, player_tws_playrate_pkt_t *playrate)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_pkt_t pkt;
    int32_t ret;

    SYS_LOG_INF("set PRate, aps: %d, time: %lld\n", playrate->aps_level, playrate->inttime);

    playrate->scene = handle->format;
    pkt.group = 0;
    pkt.type = TWS_PLAYRATE_PKT;
    pkt.payload_len = sizeof(player_tws_playrate_pkt_t);
    memcpy(pkt.payload_data, (uint8_t *)playrate, sizeof(player_tws_playrate_pkt_t));

    ret = observer->send_pkt(&pkt, sizeof(pkt));
    if(ret != sizeof(pkt))
        return -1;
    return 0;
}

/* 检测包信息: 0 表示不保存包信息单元, -1 表示需要重启播放器, 1 表示正常存储包信息单元
 */
static int32_t _tws_check_pinfo_unit(bt_tws_observer_inner_t *handle, uint16_t cur_pkt_num, uint16_t cur_pkt_len, uint16_t samples)
{
    if(cur_pkt_len == 0)
        return 0;

    if((cur_pkt_num == handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num)
        && (handle->pinfo_unit[handle->pinfo_unit_idx].pkt_len != 0))
    {
        handle->pinfo_unit[handle->pinfo_unit_idx].samples += samples;
        return 0;
    }

    if(cur_pkt_num != (handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num + 1))
    {
        /* 正在播放如果包号不连续立即重启播放(!=1:避免包号循环, !=0:避免包号开始)
         */
        if((cur_pkt_num != 1) && (handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num != 0))
            return -1;
    }

    return 1;
}

static int32_t _tws_update_sync_info(bt_tws_observer_inner_t *handle, tws_pkt_info_t *pkt_info)
{
    /* 更新idx及其内容
     */
    handle->pinfo_unit_idx ++;
    if(handle->pinfo_unit_idx > 1)
        handle->pinfo_unit_idx = 0;

    handle->pinfo_unit[handle->pinfo_unit_idx].pkt_len = pkt_info->pkt_len;
    handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num = pkt_info->pkt_num;
    handle->pinfo_unit[handle->pinfo_unit_idx].samples = pkt_info->samples;
    handle->pinfo_unit[handle->pinfo_unit_idx].pkt_bttime_us = pkt_info->pkt_bttime_us;

    /* 确保3次之后才处理SPLAY包避免开始的包不全导致计算对端播放时间出错
     */
    handle->save_pinfo_count ++;
    if(handle->save_pinfo_count > 3)
        handle->save_pinfo_count = 3;

    return 0;
}

/* 返回-1:slave_pool或maste_pool没有item
   返回 0:遍历完后没有找到相同包号的item
   返回 1:找到相同包号的item并从参数传出来
 */
static int32_t _tws_search_sinfo_item(bt_tws_observer_inner_t *handle, player_tws_syncinfo_pkt_t *slave, player_tws_syncinfo_pkt_t *maste)
{
    uint8_t slave_keep = 1, maste_keep = 1;
    player_tws_syncinfo_pkt_t slave_item, maste_item;
    uint8_t item_size = sizeof(player_tws_syncinfo_pkt_t);
    struct acts_ringbuf *slave_pool = &handle->sinfo_slave_buffer;
    struct acts_ringbuf *maste_pool = &handle->sinfo_master_buffer;

    for(;;)
    {
        if(slave_keep == 1)
        {
            if(acts_ringbuf_length(slave_pool) < item_size)
                return -1;
            
            slave_keep = 0;
            acts_ringbuf_peek(slave_pool, &slave_item, item_size);
        }

        if(maste_keep == 1)
        {
            if(acts_ringbuf_length(maste_pool) < item_size)
                return -1;
            
            maste_keep = 0;
            acts_ringbuf_peek(maste_pool, &maste_item, item_size);
        }

        if(slave_item.pkt_num > maste_item.pkt_num)
        {
            maste_keep = 1;
            acts_ringbuf_get(maste_pool, NULL, item_size);
            printf("[ATW]item mkp:%d_%d\n", maste_item.pkt_num, slave_item.pkt_num);
        }
        else if(slave_item.pkt_num < maste_item.pkt_num)
        {
            slave_keep = 1;
            acts_ringbuf_get(slave_pool, NULL, item_size);
            printf("[ATW]item skp:%d_%d\n", maste_item.pkt_num, slave_item.pkt_num);
        }
        else
        {
            printf("[ATW]item get:%d\n", maste_item.pkt_num);
            acts_ringbuf_get(maste_pool, NULL, item_size);
            acts_ringbuf_get(slave_pool, NULL, item_size);
            memcpy(slave, &slave_item, item_size);
            memcpy(maste, &maste_item, item_size);

            break;
        }
    }

    return 1;
}

static int32_t _tws_align_samples(bt_tws_observer_inner_t *handle)
{
    player_tws_syncinfo_pkt_t slave_item, maste_item;
    int32_t clk_diff;
    int32_t result = 0;
    uint8_t item_size = sizeof(player_tws_syncinfo_pkt_t);
    int8_t shake_flag;

    result = _tws_search_sinfo_item(handle, &slave_item, &maste_item);
    if(result <= 0)
    {
        handle->sync_error_count = (result == 0) ? (handle->sync_error_count + 1) : 0;
        if(handle->sync_error_count >= PLAYER_TWS_ERRCOUNT_MAXNUM)
        {
            SYS_LOG_ERR("long time no sync info");
            handle->sync_error_count = 0;
            //todo: restart playback
        }
        return 0;
    }
    
    handle->sync_error_count = 0;
    if(slave_item.bttime_us == maste_item.bttime_us)
        return 0;

    clk_diff = (int32_t)(maste_item.bttime_us - slave_item.bttime_us);
    shake_flag = (clk_diff < 0) ? 1 : -1;

    if((slave_item.pkt_num % handle->BM_TWS_Sync_interval) == 0)
    {
        handle->adjust_shake_flag = shake_flag;
        printf("[ATW]adjust e1:%d_%d\n",  slave_item.pkt_num, clk_diff);
        return 0;
    }

    if(handle->adjust_shake_flag == 0)
        return 0;
    
    if(handle->adjust_shake_flag == shake_flag)
    {
        printf("[ATW]adjust e2:%d_%d\n",  slave_item.pkt_num, clk_diff);
        handle->media_observer->notify_time_diff(handle->media_observer->media_handle, clk_diff);
    }
    
    handle->adjust_shake_flag = 0;
    return 0;
}

/* 根据本端和对端的当前信息计算对端的SPLAY
 */
static int32_t _tws_calculate_new_splay(bt_tws_observer_inner_t *handle, player_tws_startplay_pkt_t *startplay)
{
    uint8_t first_idx = (handle->pinfo_unit_idx == 1) ? 0 : 1;

    /* 要求对端以指定帧播放(当本端还没有足够的播放信息时, 让对端重新发起SPLAY)
     */
    if(handle->save_pinfo_count < 3)
    {
        printf("[ATW]need new splay\n");
        startplay->splay_flag = PLAYER_TWS_NEED_NEW_SPLAY;
        return 0;
    }
    
    startplay->splay_flag = PLAYER_TWS_USE_LOCAL_SPLAY;
    startplay->first_clk = handle->first_clk;
    if(startplay->pkt_num < (handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num + 10))
        startplay->pkt_num = handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num + 10;

    //todo: 根据已经接收的码流计算时间，sbc包长可能会变
    startplay->splay_time = handle->pinfo_unit[first_idx].pkt_bttime_us
        + (startplay->pkt_num - handle->pinfo_unit[first_idx].pkt_num)
        * handle->pinfo_unit[first_idx].samples * 1000000ll / handle->sample_rate;

    printf("[ATW]new splay:%u_%u_%u_%u, %u_%u_%u\n",
        startplay->pkt_num, startplay->first_clk,
        (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
        (uint32_t)startplay->splay_time & 0xffffffff,
        handle->pinfo_unit[first_idx].pkt_num,
        handle->first_clk,
        handle->pinfo_unit[first_idx].samples);

    return 0;
}

/* 对端要求本端发相同的SPLAY过去
 */
static int32_t _tws_handle_newly_splay(bt_tws_observer_inner_t *handle, player_tws_startplay_pkt_t *startplay, uint64_t *bttus)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;

    if((handle->tws_status == PLAYER_TWS_STATUS_PLAY)
        || (handle->tws_status == PLAYER_TWS_STATUS_DEAL_WAIT))
    {
        SYS_LOG_ERR("%u_%u, %u_%u, %u_%u",
            startplay->pkt_num, handle->first_pktnum,
            (uint32_t)(handle->splay_local_time >> 32) & 0xffffffff,
            (uint32_t)handle->splay_local_time & 0xffffffff,
            (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
            (uint32_t)startplay->splay_time & 0xffffffff);
        return 0;
    }

    startplay->splay_flag = PLAYER_TWS_USE_START_SPLAY;
    startplay->pkt_num = handle->first_pktnum;
    startplay->splay_time = bttus + handle->BM_TWS_WPlay_Mintime * 1000;
    startplay->first_clk = handle->first_clk;
    if(_tws_send_start_play(handle, &startplay) < 0)
    {
        SYS_LOG_ERR("_tws_send_start_play fail.");
        return -1;
    }

    handle->splay_local_time = startplay->splay_time;
    handle->forbit_aps_time = startplay->splay_time + 1500000;
    observer->set_interrupt_time(startplay.splay_time);

    printf("[ATW]handle new splay:%u_%u_%u\n",
        handle->first_pktnum,
        (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
        (uint32_t)startplay->splay_time & 0xffffffff);
    return 0;
}

/* 对端要求本端解码指定的包播放
 */
static int32_t _tws_handle_local_splay(bt_tws_observer_inner_t *handle, player_tws_startplay_pkt_t *startplay, uint64_t *bttus)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;

    if((handle->tws_status == PLAYER_TWS_STATUS_PLAY)
        || (handle->tws_status == PLAYER_TWS_STATUS_DEAL_WAIT))
    {
        SYS_LOG_ERR("%u_%u, %u_%u, %u_%u",
            startplay->pkt_num, handle->first_pktnum,
            (uint32_t)(handle->splay_local_time >> 32) & 0xffffffff,
            (uint32_t)handle->splay_local_time & 0xffffffff,
            (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
            (uint32_t)startplay->splay_time & 0xffffffff);
        return 0;
    }
    
    /* 对端的包号小于本端则对端需要继续播放并重新指定解码包
     */
    if((startplay->pkt_num < handle->first_pktnum)
        || (bttus >= (startplay->splay_time - TWS_MIN_INTERVAL)))
    {
        SYS_LOG_ERR("%u_%u, %u_%u, %u_%u",
            startplay->pkt_num, handle->first_pktnum,
            (uint32_t)(bttus >> 32) & 0xffffffff,
            (uint32_t)bttus & 0xffffffff,
            (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
            (uint32_t)startplay->splay_time & 0xffffffff);
        return _tws_handle_newly_splay(handle, startplay, bttus);
    }

    handle->splay_local_time = startplay.splay_time;
    handle->first_pktnum = startplay.pkt_num;
    observer->set_interrupt_time(startplay.splay_time);

    printf("[ATW]handle local splay:%u_%u_%u\n",
        handle->first_pktnum,
        (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
        (uint32_t)startplay->splay_time & 0xffffffff);

    return 0;
}

/* 对端告诉本端正在启动播放
 */
static int32_t _tws_handle_start_splay(bt_tws_observer_inner_t *handle, player_tws_startplay_pkt_t *startplay, uint64_t *bttus)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    uint32_t flags;

    if((handle->tws_status == PLAYER_TWS_STATUS_PLAY)
        || (handle->tws_status == PLAYER_TWS_STATUS_DEAL_WAIT))
    {
        /* 确定对端开始播放的蓝牙时钟
         */
        if(_tws_calculate_new_splay(handle, startplay) < 0)
        {
            SYS_LOG_ERR("_tws_calculate_new_splay fail");
            return -1;
        }

        /* 发送给对端样机
         */
        if(_tws_send_start_play(handle, startplay) < 0)
        {
            SYS_LOG_ERR("_tws_send_start_play fail.");
            return -1;
        }
        
        flags = irq_lock();
        //有接入时一段时间不能进行缓冲区调节
        handle->forbit_aps_time = bttus + 1500000;
        handle->aps_level_pending = -1;
        //对端接入后使用基准时钟播放
        handle->media_observer->set_base_aps_level(handle->media_observer->media_handle, 0xFF);
        irq_unlock(flags);

        printf("[ATW]handle start splay @1\n");
        return 0;
    }

    /* 对端包号小于当前包号, 使用当前包号, 否则使用对端包号和时间)
     */
    if((startplay->pkt_num > handle->first_pktnum)
        || ((startplay->pkt_num == handle->first_pktnum)
        && (handle->splay_local_time < startplay->splay_time)))
    {
        handle->first_pktnum = startplay->pkt_num;
        handle->splay_local_time = startplay->splay_time;
        observer->set_interrupt_time(startplay->splay_time);

        printf("[ATW]handle start splay @2:%u_%u_%u\n",
            (uint32_t)(startplay->splay_time >> 32) & 0xffffffff,
            (uint32_t)startplay->splay_time & 0xffffffff,
            startplay->pkt_num);
    }

    return 0;
}

/* 返回-1: 表示失败, 需重启播放器
 */
static void _tws_deal_startplay(bt_tws_observer_inner_t *handle, player_tws_pkt_t *tws_pkt)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_startplay_pkt_t *startplay = (player_tws_startplay_pkt_t *)(tws_pkt->payload_data);
    uint64_t bt_clk;

    if((startplay->scene != handle->format) || (tws_pkt->type != TWS_STARTPLAY_PKT))
        return;

    bt_clk = observer->get_bt_clk_us();
    if(bt_clk == 0)
    {
        SYS_LOG_ERR("get_bt_clk_us fail");
        return;
    }

    printf("[ATW]deal SPlay:%u_%u_%u_%u_%u, %u_%u_%u_%u_%u\n",
        startplay->pkt_num, startplay->first_clk,
        (uint32_t)(startplay->splay_time >> 32) & 0xffffffff, (uint32_t)startplay->splay_time & 0xffffffff,
        startplay->splay_flag,
        handle->first_pktnum, handle->first_clk,
        (uint32_t)(handle->splay_local_time >> 32) & 0xffffffff, (uint32_t)handle->splay_local_time & 0xffffffff,
        handle->tws_status);

    switch(startplay->splay_flag)
    {
    case PLAYER_TWS_NEED_NEW_SPLAY:
        _tws_handle_newly_splay(handle, startplay, bt_clk);
        break;

    case PLAYER_TWS_USE_LOCAL_SPLAY:
        _tws_handle_local_splay(handle, startplay, bt_clk);
        break;

    case PLAYER_TWS_USE_START_SPLAY:
        _tws_handle_start_splay(handle, startplay, bt_clk);
        break;

    default:
        SYS_LOG_ERR("splay_flag invalid");
        break;
    }
}

/* 返回-1: 表示失败, 需重启播放器
 */
static int32_t _tws_deal_startstop(bt_tws_observer_inner_t *handle, player_tws_pkt_t *tws_pkt)
{
    //todo
    return 0;
}

static int32_t _tws_deal_syncinfo(bt_tws_observer_inner_t *handle, player_tws_pkt_t *tws_pkt)
{
    player_tws_syncinfo_pkt_t *syncinfo = (player_tws_syncinfo_pkt_t *)(tws_pkt->payload_data);

    if((syncinfo->scene != handle->format) || (tws_pkt->type != TWS_SYNCINFO_PKT))
        return 0;

    printf("[ATW]deal SInfo:%d_%u_%u_%d\n",
        syncinfo->pkt_num,
        (uint32_t)(syncinfo->bttime_us >> 32) & 0xffffffff,
        (uint32_t) syncinfo->bttime_us & 0xffffffff,
        handle->tws_status);

    if(handle->tws_status != PLAYER_TWS_STATUS_PLAY)
        return -1;

    if(acts_ringbuf_space(&handle->sinfo_master_buffer) < sizeof(player_tws_syncinfo_pkt_t))
    {
        SYS_LOG_ERR("sinfo_master_buffer full");
        acts_ringbuf_get(&handle->sinfo_master_buffer, NULL, sizeof(player_tws_syncinfo_pkt_t));
    }
    
    acts_ringbuf_put(&handle->sinfo_master_buffer, syncinfo, sizeof(player_tws_syncinfo_pkt_t));

    return 1;
}

static int32_t _tws_deal_playrate(bt_tws_observer_inner_t *handle, player_tws_pkt_t *tws_pkt)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_playrate_pkt_t *playrate = (player_tws_playrate_pkt_t *)(tws_pkt->payload_data);
    uint32_t flags;

    if((playrate->scene != handle->format) || (tws_pkt->type != TWS_PLAYRATE_PKT))
        return 0;

    printf("[ATW]deal APSInfo:%u_%u_%u_%u\n",
        (uint32_t)(playrate->inttime >> 32) & 0xffffffff,
        (uint32_t)playrate->inttime & 0xffffffff,
        handle->tws_status,
        playrate->aps_level);

    if(handle->tws_status != PLAYER_TWS_STATUS_PLAY)
        return 0;

    /* 设置中断
     */
    observer->set_interrupt_time(playrate->inttime);

    /* 加入中断信号队列
     */
    flags = irq_lock();
    handle->aps_level_pending = playrate->aps_level;
    handle->aps_inttime = playrate.inttime;
    irq_unlock(flags);

    return 0;
}

static int32_t _tws_handle_aps_change_request(bt_tws_observer_inner_t *handle, uint8_t level)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_playrate_pkt_t playrate;
    uint32_t flags;

    if((handle->tws_role == BTSRV_TWS_SLAVE)
        || (handle->tws_status != PLAYER_TWS_STATUS_PLAY)
        || (handle->aps_level_pending == level))
        return 0;
    if(handle->tws_role == BTSRV_TWS_NONE)
        goto ERROUT;

    /* 获取蓝牙时钟
     */
    playrate.inttime = observer->get_bt_clk_us();
    if(playrate.inttime == 0)
    {
        SYS_LOG_ERR("get_bt_clk_us fail.");
        goto ERROUT;
    }
    if(playrate.inttime < handle->forbit_aps_time)
        return 0;

    /* 告诉对端调节速度
     */
    playrate.aps_level = level;
    playrate.inttime += 150000; /* 150 ms */
    if(_tws_send_play_rate(handle, &playrate) < 0)
    {
        SYS_LOG_ERR("_tws_send_play_rate fail.");
        goto ERROUT;
    }

    /* 设置中断
     */
    observer->set_interrupt_time(playrate.inttime);

    /* 加入中断信号队列
     */
    flags = irq_lock();
    handle->aps_level_pending = level;
    handle->aps_inttime = playrate.inttime;
    irq_unlock(flags);
    return 0;

ERROUT:
    handle->media_observer->set_base_aps_level(handle->media_observer->media_handle, level);
    return 0;
}

static int32_t _tws_handle_new_pkt_info(bt_tws_observer_inner_t *handle, tws_pkt_info_t *info)
{
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    int32_t ret;

    if(handle->tws_status != PLAYER_TWS_STATUS_PLAY)
        return 0;
    
    //aac两包一帧，第一包解码没有输出，清除包信息，避免时间计算不准确
    if(info->samples == 0)
    {
        SYS_LOG_INF("AAC output 0");
        goto ERROUT;
    }

    //丢弃不完整的包避免统计时间不正确
    if(handle->first_pkt_flag)
    {
        if(handle->first_pkt_flag == 1)
        {
            handle->first_pktnum = info->pkt_num;
            handle->first_pkt_flag = 2;
        }
        
        if(info->pkt_num == handle->first_pktnum)
            return 0;

        handle->first_pkt_flag = 0;
    }
    
    /* 检查包头信息
     */
    ret = _tws_check_pinfo_unit(handle, info->pkt_num, info->pkt_len, info->samples);
    if(ret == 0)
        return 0;
    if(ret < 0)
    {
        SYS_LOG_ERR("pkt miss, restart: %u_%u",
            info->pkt_num, handle->pinfo_unit[handle->pinfo_unit_idx].pkt_num);
        goto ERROUT;
    }
    
    _tws_update_sync_info(handle, info);

    if(((info->pkt_num % handle->BM_TWS_Sync_interval) == 0) || (handle->last_sinfo_flag == 1))
    {
        player_tws_syncinfo_pkt_t syncinfo;

        if(handle->last_sinfo_flag == 1)
            handle->last_sinfo_flag = 0;
        if((info->pkt_num % handle->BM_TWS_Sync_interval) == 0)
            handle->last_sinfo_flag = 1;

        memset(&syncinfo, 0, sizeof(player_tws_syncinfo_pkt_t));
        syncinfo.pkt_num = info->pkt_num;
        syncinfo.bttime_us = handle->pinfo_unit[handle->pinfo_unit_idx].pkt_bttime_us;

        if(acts_ringbuf_space(&handle->sinfo_slave_buffer) < sizeof(player_tws_syncinfo_pkt_t))
            acts_ringbuf_get(&handle->sinfo_slave_buffer, NULL, sizeof(player_tws_syncinfo_pkt_t));
        acts_ringbuf_put(&handle->sinfo_slave_buffer, &syncinfo, sizeof(player_tws_syncinfo_pkt_t));

        if(handle->tws_role == BTSRV_TWS_MASTER)
            _tws_send_sync_info(handle, &syncinfo);

        printf("[ATW]SInfo:%u_%d_%u_%u\n", info->pkt_num, info->pkt_len,
            (uint32_t)(syncinfo.bttime_us >> 32) & 0xffffffff, (uint32_t)syncinfo.bttime_us & 0xffffffff);
    }

    return 0;

ERROUT:
    handle->pinfo_unit_idx = 0;
    handle->save_pinfo_count = 0;
    memset(handle->pinfo_unit, 0, sizeof(handle->pinfo_unit));

    //丢包重启播放
    if((info->samples != 0) && (handle->tws_role != BTSRV_TWS_NONE))
        ;//todo: restart playback
    return 0;
}

static void _bt_tws_loop(struct k_timer *timer)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    int32_t role;

    role = observer->get_role();
    if(role != handle->tws_role)
    {
        acts_ringbuf_drop_all(&handle->sinfo_master_buffer);
        
        handle->aps_level_pending = -1;
        handle->forbit_aps_time = observer->get_bt_clk_us() + 1500000;
        handle->tws_role = role;
    }

    while(acts_ringbuf_length(&handle->tws_data_buffer) >= sizeof(tws_pkt))
    {
        player_tws_pkt_t tws_pkt;
        
        acts_ringbuf_get(&handle->tws_data_buffer, &tws_pkt, sizeof(tws_pkt));
      
        switch(tws_pkt.type)
        {
        case TWS_STARTPLAY_PKT:
            _tws_deal_startplay(handle, tws_pkt);
            break;
        case TWS_STARTSTOP_PKT:
            _tws_deal_startstop(handle, tws_pkt);
            break;
        case TWS_SYNCINFO_PKT:
            if(role == BTSRV_TWS_SLAVE)
                _tws_deal_syncinfo(handle, tws_pkt);
            break;
        case TWS_PLAYRATE_PKT:
            if(role == BTSRV_TWS_SLAVE)
                _tws_deal_playrate(handle, tws_pkt);
            break;
        default:
            SYS_LOG_ERR("recv invalid pkt, group: %d, type: %d, len: %d",
                tws_pkt.group, tws_pkt.type, tws_pkt.payload_len);
            break;
        }
    }

    while(acts_ringbuf_length(&handle->msg_buffer) >= sizeof(player_tws_msg_t))
    {
        player_tws_msg_t tws_msg;
        
        acts_ringbuf_get(&handle->msg_buffer, &tws_msg, sizeof(tws_msg));
      
        switch(tws_msg.type)
        {
        case TWS_SYNCINFO_PKT:
            _tws_handle_new_pkt_info(handle, (tws_pkt_info_t*)tws_msg.data);
            break;
        case TWS_PLAYRATE_PKT:
            _tws_handle_aps_change_request(handle, tws_msg.data[0]);
            break;
        default:
            SYS_LOG_ERR("recv invalid msg, type: %d", tws_msg.type);
            break;
        }
    }

    if((handle->tws_status == PLAYER_TWS_STATUS_PLAY) && (role == BTSRV_TWS_SLAVE))
        _tws_align_samples(handle);

    if(handle->tws_status == PLAYER_TWS_STATUS_WAIT)
    {
        uint64_t bt_clk;
        
        bt_clk = observer->get_bt_clk_us();
        if(bt_clk == 0)
        {
            SYS_LOG_ERR("get_bt_clk_us fail");
            return;
        }

        bt_clk += TWS_MIN_INTERVAL;
        if(bt_clk >= handle->splay_local_time)
        {
            handle->tws_status == PLAYER_TWS_STATUS_DEAL_WAIT;
            handle->media_observer->set_start_pkt_num(handle->media_observer, handle->first_pktnum);
        }
    }
}

/*----------------------------------------------------------------------------------------------------*/

static uint64_t _bt_tws_get_bt_clk_us(void)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;

    return observer->get_bt_clk_us();
}

static uint8_t _bt_tws_get_role(void)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    return handle->tws_role;
}

static int32_t _bt_tws_set_stream_info(uint8_t format, uint16_t first_pktnum, uint16_t sample_rate)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)handle->media_observer->tws_observer;
    player_tws_startplay_pkt_t startplay;

    if(handle->tws_status != PLAYER_TWS_STATUS_INIT)
    {
        SYS_LOG_ERR("bt tws status: %d", handle->tws_status);
        return -1;
    }
    
    handle->format = format;
    handle->first_pktnum = first_pktnum;
    handle->sample_rate = sample_rate;
    handle->first_clk = observer->get_first_pkt_clk();
    
    handle->tws_status = PLAYER_TWS_STATUS_WAIT;
    handle->aps_level_pending = -1;
    handle->first_pkt_flag = 1;
    
    if(format == MSBC_TYPE || format == CVSD_TYPE)
    {
        handle->BM_TWS_WPlay_Mintime = 80;
        handle->BM_TWS_WPlay_Maxtime = 600;
        handle->BM_TWS_Sync_interval = 200;
    }
    else
    {
        handle->BM_TWS_WPlay_Mintime = 100;
        handle->BM_TWS_WPlay_Maxtime = 1000;
        handle->BM_TWS_Sync_interval = 40;
    }

    if(handle->tws_role == BTSRV_TWS_NONE)
        goto ERROUT;

    startplay.splay_flag = PLAYER_TWS_USE_START_SPLAY;
    startplay.pkt_num = first_pktnum;
    startplay.splay_time = observer->get_bt_clk_us() + handle->BM_TWS_WPlay_Mintime * 1000;
    startplay.first_clk = handle->first_clk;
    if(_tws_send_start_play(handle, &startplay) < 0)
    {
        SYS_LOG_ERR("_tws_send_start_play fail.");
        goto ERROUT;
    }

    handle->splay_local_time = startplay.splay_time;
    handle->forbit_aps_time = startplay.splay_time + 1500000;
    observer->set_interrupt_time(startplay.splay_time);

    os_timer_start(&handle->timer, BT_TWS_TIMER_INTERVAL, BT_TWS_TIMER_INTERVAL);
    
    return 0;

ERROUT:
    handle->tws_status = PLAYER_TWS_STATUS_PLAY;
    handle->media_observer->set_start_pkt_num(handle->media_observer->media_handle, first_pktnum);
    handle->media_observer->start_playback(handle->media_observer->media_handle);
    return 0;
}

static int32_t _bt_tws_aps_change_request(uint8_t level)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    player_tws_msg_t tws_msg;
    int32_t wait_time = 5;

    if(handle->tws_status == PLAYER_TWS_STATUS_INIT)
        return -1;

    tws_msg.type = TWS_PLAYRATE_PKT;
    tws_msg.data[0] = level;

    while(acts_ringbuf_space(&handle->msg_buffer) < sizeof(player_tws_msg_t))
    {
        if(wait_time <= 0)
        {
            SYS_LOG_ERR("msg buffer full");
            return -1;
        }
        
        os_sleep(1);
        wait_time--;
    }
    
    acts_ringbuf_put(&handle->msg_buffer, &tws_msg, sizeof(tws_msg));
    return 0;
}

static int32_t _bt_tws_set_pkt_info(tws_pkt_info_t *info)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    player_tws_msg_t tws_msg;
    int32_t wait_time = 5;

    if(handle->tws_status == PLAYER_TWS_STATUS_INIT)
        return -1;

    tws_msg.type = TWS_SYNCINFO_PKT;
    memcpy(tws_msg.data, info, sizeof(tws_pkt_info_t));

    while(acts_ringbuf_space(&handle->msg_buffer) < sizeof(player_tws_msg_t))
    {
        if(wait_time <= 0)
        {
            SYS_LOG_ERR("msg buffer full");
            return -1;
        }
        
        os_sleep(1);
        wait_time--;
    }
    
    acts_ringbuf_put(&handle->msg_buffer, &tws_msg, sizeof(tws_msg));
    return 0;
}

bt_tws_observer_t* bluetooth_tws_observer_init(media_observer_t *media_observer)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)media_observer->tws_observer;

    memset(handle, 0, sizeof(bt_tws_observer_inner_t));
    
    acts_ringbuf_init(&handle->tws_data_buffer, handle->tws_data, sizeof(handle->tws_data));
    acts_ringbuf_init(&handle->sinfo_master_buffer, handle->sinfo_master, sizeof(handle->sinfo_master));
    acts_ringbuf_init(&handle->sinfo_slave_buffer, handle->sinfo_slave, sizeof(handle->sinfo_slave));
    acts_ringbuf_init(&handle->msg_buffer, handle->msg_data, sizeof(handle->msg_data));
    
    handle->media_observer = media_observer;
    handle->tws_observer.get_bt_clk_us = _bt_tws_get_bt_clk_us;
    handle->tws_observer.get_role = _bt_tws_get_role;
    handle->tws_observer.set_stream_info = _bt_tws_set_stream_info;
    handle->tws_observer.aps_change_request = _bt_tws_aps_change_request;
    handle->tws_observer.set_pkt_info = _bt_tws_set_pkt_info;
    handle->tws_status = PLAYER_TWS_STATUS_INIT;
    handle->aps_level_pending = -1;
    handle->tws_role = observer->get_role();
    os_timer_init(&handle->timer, _bt_tws_loop, NULL);

    observer->set_interrupt_cb(_tws_irq_handle);
    observer->set_recv_pkt_cb(_tws_recv_pkt_handle);
    SYS_LOG_ERR("sizeof(player_tws_msg_t): %d, %d", sizeof(player_tws_msg_t), sizeof(tws_pkt_info_t));

    return &handle->tws_observer;
}

void bluetooth_tws_observer_deinit(bt_tws_observer_t *tws_observer)
{
    bt_tws_observer_inner_t *handle = &observer_inner;
    tws_runtime_observer_t *observer = (tws_runtime_observer_t *)media_observer->tws_observer;
    uint32_t flags;

    flags = irq_lock();
    handle->aps_level_pending = -1;
    handle->tws_status = PLAYER_TWS_STATUS_INIT;
    irq_unlock(flags);

    os_timer_stop(&handle->timer);
    observer->set_interrupt_time(0);
    observer->set_interrupt_cb(NULL);
    observer->set_recv_pkt_cb(NULL);
}

