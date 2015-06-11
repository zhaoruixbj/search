/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: crwl_conf.h
 ** 版本号: 1.0
 ** 描  述: 爬虫配置
 **         定义爬虫配置相关的结构体
 ** 作  者: # Qifeng.zou # 2014.09.04 #
 ******************************************************************************/
#if !defined(__CRWL_CONF_H__)
#define __CRWL_CONF_H__

#include "comm.h"
#include "redis.h"
#include "xml_tree.h"
#include "mem_pool.h"

/* Worker配置信息 */
typedef struct
{
    int num;                                /* 爬虫线程数 */
    int conn_max_num;                       /* 并发网页连接数 */
    int conn_tmout_sec;                     /* 连接超时时间 */
} crwl_worker_conf_t;

/* Parser配置信息 */
typedef struct
{
    struct
    {
        char path[FILE_PATH_MAX_LEN];       /* 数据存储路径 */
        char err_path[FILE_PATH_MAX_LEN];   /* 错误数据存储路径 */
    } store;
} crwl_filter_conf_t;

/* Seed配置信息 */
typedef struct
{
    char uri[URI_MAX_LEN];                  /* 网页URI */
    unsigned int depth;                     /* 网页深度 */
} crwl_seed_conf_t;

/* Redis配置信息 */
typedef struct
{
    int num;                                /* 配置数目 */
    redis_conf_t *conf;                     /* 配置数组(注: [0]为MASTER配置 [1~N]为副本配置 */

    char taskq[QUEUE_NAME_MAX_LEN];         /* 任务队列名 */
    char done_tab[TABLE_NAME_MAX_LEN];      /* DONE哈希表名 */
    char push_tab[TABLE_NAME_MAX_LEN];      /* PUSHED哈希表名 */
} crwl_redis_conf_t;

/* 爬虫配置信息 */
typedef struct
{
    struct
    {
        int level;                          /* 日志级别 */
        char path[FILE_PATH_MAX_LEN];       /* 日志路径 */
    } log;                                  /* 日志配置 */

    struct
    {
        unsigned int depth;                 /* 最大爬取深度 */
        char path[FILE_PATH_MAX_LEN];       /* 网页存储路径 */
    } download;                             /* 下载配置 */

    int workq_count;                        /* 工作队列容量 */
    int man_port;                           /* 管理服务侦听端口 */

    crwl_redis_conf_t redis;                /* REDIS配置 */
    crwl_worker_conf_t worker;              /* WORKER配置 */

    bool sched_stat;                        /* 调度状态(false:暂停 true:运行) */
} crwl_conf_t;

crwl_conf_t *crwl_conf_load(const char *path, log_cycle_t *log);
#define crwl_conf_destroy(conf)             /* 销毁配置对象 */\
{ \
    free(conf); \
}

#endif /*__CRWL_CONF_H__*/
