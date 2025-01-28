/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief cpu load statistic
 */
#if defined(CONFIG_SOC_SPICACHE_PROFILE)

#include <shell/shell.h>
#include <init.h>
#include <stdlib.h>
#include <string.h>
#include <spicache.h>

static struct spicache_profile profile_data;

#define RUNNING_CYCLES(end, start)	((uint32_t)((long)(end) - (long)(start)))

/* start flag */
static int cacheprofile_started;

/* cpu load poll interval, unit: ms */
static int cacheprofile_interval;

struct k_delayed_work cacheprofile_stat_work;

void cacheprofile_stat(uint32_t interval);

void spicache_stat_profile_dump(struct spicache_profile *profile)
{
    u32_t interval_ms;
    u64_t hit_cnt, total;

    if (!profile)
        return;

    interval_ms = k_cyc_to_ns_floor64((profile->end_time - profile->start_time)) / 1000000;

    if (profile->spi_id == 0) {
        hit_cnt = (u64_t)profile->hit_cnt * 8;
        total = hit_cnt + profile->miss_cnt;
        if (total != 0){
            printk("spi%d cache profile: addr range 0x%08x ~ 0x%08x, profile time %u ms\n",
                profile->spi_id, profile->start_addr, profile->end_addr, interval_ms);

            printk("range hit: %u,%u  miss: %u	miss ratio: %u %%%% \n",
                (u32_t)(hit_cnt >> 32), (u32_t)hit_cnt,
                profile->miss_cnt,
                (u32_t)(((u64_t)profile->miss_cnt * 100 * 100) / total));

            hit_cnt = (u64_t)profile->total_hit_cnt * 8;
            total = hit_cnt + profile->total_miss_cnt;
            if (total != 0)
                printk("total hit: %u,%u  miss: %u  miss time ratio: %u %%%% \n",
                    (u32_t)(hit_cnt >> 32), (u32_t)hit_cnt, profile->total_miss_cnt,
                    (u32_t)(((u64_t)profile->total_miss_cnt * 34 * 80 / (4 * 100)) / interval_ms));
        }
    }
}

/*
 * cmd: spicache_profile
 *   start start_addr end_addr
 *   stop
 */
int shell_cmd_spicache_profile(int argc, char *argv[])
{
    struct spicache_profile *profile;
    int len = strlen(argv[1]);

    profile = &profile_data;
    if (!strncmp(argv[1], "start", len)) {
        if (argc < 5)
            return -EINVAL;

        memset(profile, 0, sizeof(struct spicache_profile));

        profile->start_addr = strtoul(argv[2], NULL, 0);
        profile->end_addr = strtoul(argv[3], NULL, 0);
        profile->spi_id = strtoul(argv[4], NULL, 0);

        printk("Start profile: addr range %08x ~ %08x\n",
            profile->start_addr, profile->end_addr);

        spicache_profile_start(profile);

    } else if (!strncmp(argv[1], "stop", len)) {
        printk("Stop profile\n");
        spicache_profile_stop(profile);
        spicache_stat_profile_dump(profile);
    } else {
        printk("usage:\n");
        printk("  spicache_profile start start_addr end_addr 0/1 \n");
        printk("  spicache_profile stop\n");

        return -EINVAL;
    }

    return 0;
}

static void cacheprofile_stat_clear(void)
{
    struct spicache_profile *profile;
    profile = &profile_data;

    unsigned int key;

    key = irq_lock();

    memset(profile, 0, sizeof(struct spicache_profile));

    irq_unlock(key);
}


static void cacheprofile_stat_callback(struct k_work *work)
{
    cacheprofile_stat(cacheprofile_interval);
}

void cacheprofile_stat_start(int interval_ms, u32_t start_addr, u32_t end_addr, u32_t step_addr, u32_t profile_id)
{
    struct spicache_profile *profile;
    profile = &profile_data;

    if (!start_addr || !end_addr || !step_addr || !interval_ms || (interval_ms < 500))
        return;

    if (start_addr >= end_addr || (start_addr + step_addr) >= end_addr)
        return;

    if (cacheprofile_started)
        k_delayed_work_cancel(&cacheprofile_stat_work);

    printk("Start profile stat: addr range %08x ~ %08x step %08x interval:%08x ms\n",
        start_addr, end_addr, step_addr, interval_ms);

    cacheprofile_stat_clear();

    cacheprofile_interval = interval_ms;
    cacheprofile_started = 1;

    profile->start_addr = start_addr;
    profile->end_addr = start_addr + step_addr;
    profile->limit_addr = end_addr;
    profile->spi_id = profile_id;

    spicache_profile_start(profile);

    k_delayed_work_init(&cacheprofile_stat_work, cacheprofile_stat_callback);
    k_delayed_work_submit(&cacheprofile_stat_work, K_MSEC(interval_ms));
}

void cacheprofile_stat_stop(void)
{
    k_delayed_work_cancel(&cacheprofile_stat_work);
    cacheprofile_started = 0;
}

void cacheprofile_stat(uint32_t interval)
{
    u32_t step_addr;
    struct spicache_profile *profile;
    profile = &profile_data;

    spicache_profile_stop(profile);
    spicache_stat_profile_dump(profile);

    step_addr = profile->end_addr - profile->start_addr;

    printk("profile end %x step %x limit %x\n", profile->end_addr, step_addr, \
        profile->limit_addr);

    if(profile->end_addr + step_addr <= profile->limit_addr){
        profile->start_addr = profile->end_addr;
        profile->end_addr += step_addr;
        spicache_profile_start(profile);
        k_delayed_work_submit(&cacheprofile_stat_work, K_MSEC(cacheprofile_interval));
    }else{
        cacheprofile_stat_stop();
        printk("Stop profile\n");
    }
}

/*
 * cmd: spicache_profile
 *   start start_addr end_addr
 *   stop
 */
int shell_cmd_spicache_profile_stat(const struct shell *shell,
                  size_t argc, char **argv)
{
    struct spicache_profile *profile;
    int len = strlen(argv[1]);

    u32_t start_addr, end_addr, step_addr, spi_id, interval_ms;

    profile = &profile_data;
    if (!strncmp(argv[1], "start", len)) {
        printk("argc %d\n", argc);
        if (argc < 7)
            return -EINVAL;

        start_addr = strtoul(argv[2], NULL, 0);
        end_addr = strtoul(argv[3], NULL, 0);
        step_addr = strtoul(argv[4], NULL, 0);
        spi_id = strtoul(argv[5], NULL, 0);
        interval_ms = strtoul(argv[6], NULL, 0);

        cacheprofile_stat_start(interval_ms, start_addr, end_addr, step_addr, spi_id);

    } else if (!strncmp(argv[1], "stop", len)) {
        cacheprofile_stat_stop();
    } else {
        printk("usage:\n");
        printk("  spicache_stat start start_addr end_addr step_addr 0/1 interval_ms\n");
        printk("  spicache_stat stop\n");

        return -EINVAL;
    }

    return 0;
}


#endif	/* CONFIG_SOC_SPICACHE_PROFILE */

