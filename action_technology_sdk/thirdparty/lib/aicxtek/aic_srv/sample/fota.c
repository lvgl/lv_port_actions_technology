/**************************************************************************/
/*                                                                        */
/* Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file aic ctrl module source file.
 */
#include <alog_api.h>
#include <aic_osa.h>
#include <aic_errno.h>
#include <aic_ctrl.h>
#include <aic_fota.h>
#include <aic_posix.h>

/*
 * CONF_FOTA_POWER_DOWN_RESTORE -
 * Enable it can save the fota update state to flash,
 * And if the device lost power during the fota upgrade process,
 * It can resume upgrades after the power restore.
 *
 * Open it may need to reimplemented the function aic_fota_test_load_update_result
 * and the function aic_fota_test_save_fupdate_result.
 */
#define CONF_FOTA_POWER_DOWN_RESTORE       0

#if CONF_FOTA_POWER_DOWN_RESTORE
#include <user_cfg_id.h>
#endif

/*
 * if enable it will test the fota stop api in fota update flow.
 */
#define CONFIG_TEST_AUTO_STOP           0

#define AIC_FOTA_TEST_TASK_PRI          20
#define AIC_FOTA_TEST_UPDATE_MAX_TIME   200
#define ONCE_TRANS_SIZE                 2048

#define FTST_ERR(fmt, ...)      alog_error("ftst: "fmt, ##__VA_ARGS__)
#define AFOT_WARN(fmt, ...)     alog_warn("ftst: "fmt, ##__VA_ARGS__)
#define FTST_INFO(fmt, ...)     alog_info("ftst: "fmt, ##__VA_ARGS__)
#define FTST_DBG(fmt, ...)      alog_debug("ftst: "fmt, ##__VA_ARGS__)

typedef struct fota_update_cfg {
    aic_fota_update_result_t    result;
} fota_update_cfg_t;

static pthread_t            fota_test_task;

#if CONF_FOTA_POWER_DOWN_RESTORE
static fota_update_cfg_t    fota_cfg;
#endif
u8 pac_sim_data[] = {
	#include "diff_pac.c"
};

int aic_fota_get_pac_by_version(int old_ver, int new_ver,
                                u8 **pp_pac_data, u32 *p_pac_size)
{
    FTST_INFO("get pac, old=%d, new=%d.\n", old_ver, new_ver);

    /*
     * TODO:
     * here just spcify the version data.
     * Customers need to implemt it acoording their own version
     * managemant method.
     */

    *pp_pac_data = pac_sim_data;
    *p_pac_size = sizeof(pac_sim_data);

    return AIC_SUCCESS;
}

#if CONF_FOTA_POWER_DOWN_RESTORE
static void aic_fota_test_save_fupdate_result()
{
    int ret = syscfg_write(VM_FOTA_UPDATE_CFG, &fota_cfg, sizeof(fota_cfg));

    FTST_INFO("save result=%d, ret=%d", fota_cfg.result, ret);
    AIC_ASSERT(ret == sizeof(fota_cfg));
}

static void aic_fota_test_load_update_result(void)
{
    int ret =  syscfg_read(VM_FOTA_UPDATE_CFG, &fota_cfg, sizeof(fota_cfg));

    FTST_INFO("load result=%d, ret=%d", fota_cfg.result, ret);
}
#endif

void* aic_fota_test_task(void *p)
{
    u32 pac_size, trans_size, trans_total;
    aic_fota_update_result_t ret;
    aic_ctrl_inter_msg_t msg;
    aic_ctrl_state_t state;
    int local_ver, remote_ver;
    u8 *pac_data;
#if CONFIG_TEST_AUTO_STOP
    int retry = 0;
#endif

    sleep(10);

    FTST_INFO("test task.\n");

    local_ver = aic_fota_get_aic_local_ver_num();
    remote_ver = aic_fota_get_aic_remote_ver_num();

    FTST_INFO("local_ver=%d, remote_ver=%d.\n", local_ver, remote_ver);

    if (remote_ver < 0) {
        FTST_ERR("Get remote ver err, exit fota.\n", local_ver, remote_ver);
        return NULL;
    }

#if CONF_FOTA_POWER_DOWN_RESTORE
    /*
     * Only local_ver == remote_ver, and the fota_cfg is complete, can skip the
     * fota update flow. Because the aic software has 2 part: basic img + extend
     * img, and the aic version is stored in baisc image. if the sdk_ver == aic_ver
     * is just indicate that the basic image has update succ, but the extend image
     * may be old software.
     */
    if (local_ver == remote_ver) {
        /* get the update_result from flash. */
        aic_fota_test_load_update_result();

        /* result must is AIC_FOTA_UPDATE_COMPLETE. */
        if (fota_cfg.result == AIC_FOTA_UPDATE_COMPLETE) {
            FTST_INFO("aic is the latest version! .\n");
            return NULL;
        }

        /* continue to frimware update. */
        FTST_INFO("aic update continue! .\n");
    }
#endif

    while (TRUE) {
        /* get the pac by version. */
        ret = aic_fota_get_pac_by_version(remote_ver, local_ver, &pac_data, &pac_size);
        AIC_ASSERT(ret == AIC_SUCCESS);

        /* start the fota update. */
        ret = aic_fota_start(pac_size);
        AIC_ASSERT(ret == AIC_SUCCESS);

        /* trans, fota data. */
        trans_total = 0;
        do {
            trans_size = MIN(pac_size, ONCE_TRANS_SIZE);
            ret = aic_fota_trans_data(pac_data + trans_total, trans_size);

            if (ret != AIC_SUCCESS)
                break;

#if CONFIG_TEST_AUTO_STOP
            /* rety 3 times to test fota stop. */
            if (trans_total > ONCE_TRANS_SIZE * 4  && retry < 3) {
                retry++;
                break;
            }
#endif
            pac_size -= trans_size;
            trans_total += trans_size;
        } while (pac_size > 0);

        /* rety 3 times to test fota stop. */
        if (pac_size > 0) {
            aic_fota_stop();
            sleep(10);
            continue;
        }

        /* wait fota update result. */
#if CONF_FOTA_POWER_DOWN_RESTORE
        /* need save the int result fo flash. */
        fota_cfg.result = AIC_FOTA_UPDATE_INIT;
        aic_fota_test_save_fupdate_result();
#endif

        ret = aic_fota_wait_update_result(AIC_FOTA_TEST_UPDATE_MAX_TIME);

#if CONF_FOTA_POWER_DOWN_RESTORE
        /* need save the update result fo flash. */
        fota_cfg.result = ret;
        aic_fota_test_save_fupdate_result();
#endif

        /* stop the fota update. */
         aic_fota_stop();

        if (ret == AIC_FOTA_UPDATE_COMPLETE) {
            FTST_INFO("update succ!\n");
            break;
        }

        if (ret == AIC_FOTA_PAC_VER_NOT_MATCH) {
            FTST_INFO("version not matched, please check!\n");
            break;
        }

        FTST_INFO("update failed(ret=%d), retry later!\n", ret);
        sleep(20);
    }

    return (void *)0;
}

void aic_fota_test_init(void)
{
    pthread_attr_t attr = {0};
    struct sched_param param;
    int ret;

    FTST_INFO("test init.\n");

    pthread_attr_init(&attr);
    param.sched_priority = AIC_FOTA_TEST_TASK_PRI;
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&fota_test_task, &attr, aic_fota_test_task, NULL);
    AIC_ASSERT(!ret);
}


