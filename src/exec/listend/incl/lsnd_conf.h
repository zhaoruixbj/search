#if !defined(__LSND_CONF_H__)
#define __LSND_CONF_H__

#include "comm.h"
#include "agent.h"
#include "rtsd_send.h"

/* 代理配置 */
typedef struct
{
    unsigned int nodeid;            /* 结点ID */
    int log_level;                  /* 日志级别 */
    agent_conf_t agent;             /* 代理配置 */
    rtsd_conf_t to_frwd;            /* 转发配置 */
} lsnd_conf_t;

int lsnd_load_conf(const char *path, lsnd_conf_t *conf, log_cycle_t *log);

#endif /*__AGENTD_CONF_H__*/
