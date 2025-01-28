/**
 * @file stack_check_cfg.h
 * Check config for lib and open source
 */

#ifdef CONFIG_SMP
#define CFG_SMP_BIT                 (0x1 << 0)
#else
#define CFG_SMP_BIT                 (0)
#endif

#ifdef CONFIG_SPIN_VALIDATE
#define CFG_SPIN_VALIDATE_BIT       (0x1 << 1)
#else
#define CFG_SPIN_VALIDATE_BIT       (0)
#endif

#ifdef CONFIG_WAITQ_SCALABLE
#define CFG_WAITQ_SCALABLE_BIT      (0x1 << 2)
#else
#define CFG_WAITQ_SCALABLE_BIT      (0)
#endif

#ifdef CONFIG_MISRA_SANE
#define CFG_MISRA_SANE_BIT          (0x1 << 3)
#else
#define CFG_MISRA_SANE_BIT          (0)
#endif

#ifdef CONFIG_POLL
#define CFG_POLL_BIT                (0x1 << 4)
#else
#define CFG_POLL_BIT                (0)
#endif

#ifdef CONFIG_TIMEOUT_64BIT
#define CFG_TIMEOUT_64BIT_BIT       (0x1 << 5)
#else
#define CFG_TIMEOUT_64BIT_BIT       (0)
#endif

#ifdef CONFIG_BT_USER_PHY_UPDATE
#define CFG_BT_USER_PHY_UPDATE_BIT          (0x1 << 6)
#else
#define CFG_BT_USER_PHY_UPDATE_BIT          (0)
#endif

#ifdef CONFIG_BT_USER_DATA_LEN_UPDATE
#define CFG_BT_USER_DATA_LEN_UPDATE_BIT     (0x1 << 7)
#else
#define CFG_BT_USER_DATA_LEN_UPDATE_BIT     (0)
#endif

#ifdef CONFIG_BT_AUDIO
#define CFG_BT_AUDIO_BIT                    (0x1 << 9)
#else
#define CFG_BT_AUDIO_BIT                    (0)
#endif

#ifdef CONFIG_BT_REMOTE_VERSION
#define CFG_BT_REMOTE_VERSION_BIT           (0x1 << 10)
#else
#define CFG_BT_REMOTE_VERSION_BIT           (0)
#endif

#ifdef CONFIG_BT_SMP
#define CFG_BT_SMP_BIT                      (0x1 << 11)
#else
#define CFG_BT_SMP_BIT                      (0)
#endif

#ifdef CONFIG_BT_REMOTE_INFO
#define CFG_BT_REMOTE_INFO_BIT              (0x1 << 12)
#else
#define CFG_BT_REMOTE_INFO_BIT              (0)
#endif

#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
#define CFG_BT_SMP_APP_PAIRING_ACCEPT_BIT   (0x1 << 13)
#else
#define CFG_BT_SMP_APP_PAIRING_ACCEPT_BIT   (0)
#endif

#ifdef CONFIG_NET_BUF_POOL_USAGE
#define CFG_NET_BUF_POOL_USAGE_BIT          (0x1 << 14)
#else
#define CFG_NET_BUF_POOL_USAGE_BIT          (0)
#endif

#ifdef CONFIG_NET_BUF_LOG
#define CFG_NET_BUF_LOG_BIT                 (0x1 << 15)
#else
#define CFG_NET_BUF_LOG_BIT                 (0)
#endif

#ifdef CONFIG_BT_SETTINGS
#define CFG_BT_SETTINGS_BIT                 (0x1 << 16)
#else
#define CFG_BT_SETTINGS_BIT                 (0)
#endif

#ifdef CONFIG_BT_CENTRAL
#define CFG_BT_CENTRAL_BIT                  (0x1 << 17)
#else
#define CFG_BT_CENTRAL_BIT                  (0)
#endif

#ifdef CONFIG_BT_SIGNING
#define CFG_BT_SIGNING_BIT                  (0x1 << 18)
#else
#define CFG_BT_SIGNING_BIT                  (0)
#endif

#ifdef CONFIG_BT_SMP_SC_PAIR_ONLY
#define CFG_BT_SMP_SC_PAIR_ONLY_BIT         (0x1 << 19)
#else
#define CFG_BT_SMP_SC_PAIR_ONLY_BIT         (0)
#endif

#ifdef CONFIG_BT_PROPERTY
#define CFG_BT_PROPERTY_BIT                 (0x1 << 20)
#else
#define CFG_BT_PROPERTY_BIT                 (0)
#endif


#define STACK_CHECK_CFG_VALUE				(CFG_SMP_BIT | \
											CFG_SPIN_VALIDATE_BIT | \
											CFG_WAITQ_SCALABLE_BIT | \
											CFG_MISRA_SANE_BIT | \
											CFG_POLL_BIT | \
											CFG_TIMEOUT_64BIT_BIT | \
											CFG_BT_USER_PHY_UPDATE_BIT | \
											CFG_BT_USER_DATA_LEN_UPDATE_BIT | \
											CFG_BT_AUDIO_BIT | \
											CFG_BT_REMOTE_VERSION_BIT | \
											CFG_BT_SMP_BIT | \
											CFG_BT_REMOTE_INFO_BIT | \
											CFG_BT_SMP_APP_PAIRING_ACCEPT_BIT | \
											CFG_NET_BUF_POOL_USAGE_BIT | \
											CFG_NET_BUF_LOG_BIT | \
											CFG_BT_SETTINGS_BIT | \
											CFG_BT_CENTRAL_BIT | \
											CFG_BT_SIGNING_BIT | \
											CFG_BT_SMP_SC_PAIR_ONLY_BIT | \
											CFG_BT_PROPERTY_BIT)
