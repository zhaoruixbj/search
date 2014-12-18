#if !defined(__SEARCH_H__)
#define __SEARCH_H__

#include "queue.h"
#include "srch_conf.h"
#include "thread_pool.h"

/* 宏定义 */
#define SRCH_TMOUT_SCAN_SEC         (05)    /* 超时扫描间隔 */
#define SRCH_CONNECT_TMOUT_SEC      (00)    /* 连接超时时间 */

#define SRCH_THD_MAX_NUM            (64)    /* 最大线程数 */
#define SRCH_THD_DEF_NUM            (05)    /* 默认线程数 */
#define SRCH_THD_MIN_NUM            (01)    /* 最小线程数 */

#define SRCH_SLAB_SIZE              (30 * MB)   /* SLAB内存池大小 */
#define SRCH_RECV_SIZE              (8 * KB)    /* 缓存SIZE(接收缓存) */
#define SRCH_SYNC_SIZE  (SRCH_RECV_SIZE >> 1)   /* 缓存同步SIZE */

#define SRCH_CONN_MAX_NUM           (1024)  /* 最大网络连接数 */
#define SRCH_CONN_DEF_NUM           (128)   /* 默认网络连接数 */
#define SRCH_CONN_MIN_NUM           (1)     /* 最小网络连接数 */
#define SRCH_CONN_TMOUT_SEC         (15)    /* 连接超时时间(秒) */

#define SRCH_CONNQ_LEN              (10000) /* 连接队列长度 */
#define SRCH_RECVQ_LEN              (10000) /* 接收队列长度 */
#define SRCH_RECVQ_SIZE             (2 * KB)/* 接收队列SIZE */

#define SRCH_DEF_CONF_PATH  "../conf/search.xml"   /* 默认配置路径 */

/* 命令路径 */
#define SRCH_LSN_CMD_PATH "../temp/srch/lsn_cmd.usck"       /* 侦听线程 */
#define SRCH_RCV_CMD_PATH "../temp/srch/rcv_cmd_%02d.usck"  /* 接收线程 */
#define SRCH_WRK_CMD_PATH "../temp/srch/wrk_cmd_%02d.usck"  /* 工作线程 */

/* 错误码 */
typedef enum
{
    SRCH_OK = 0                             /* 正常 */
    , SRCH_SHOW_HELP                        /* 显示帮助信息 */
    , SRCH_DONE                             /* 完成 */
    , SRCH_SCK_AGAIN                        /* 出现EAGAIN提示 */
    , SRCH_SCK_CLOSE                        /* 套接字关闭 */

    , SRCH_ERR = ~0x7FFFFFFF                /* 失败、错误 */
} srch_err_code_e;

/* 输入参数 */
typedef struct
{
    char conf_path[FILE_NAME_MAX_LEN];      /* 配置文件路径 */
    bool isdaemon;                          /* 是否后台运行 */
} srch_opt_t;

/* 搜索引擎对象 */
typedef struct
{
    srch_conf_t *conf;                      /* 配置信息 */
    log_cycle_t *log;                       /* 日志对象 */

    pthread_t lsn_tid;                      /* Listen线程 */
    thread_pool_t *agents;                  /* Agent线程池 */
    thread_pool_t *workers;                 /* Worker线程池 */

    queue_t **connq;                        /* 连接队列(注:数组长度与Recver相等) */
    queue_t **recvq;                        /* 接收队列(注:数组长度与Worker相等) */
} srch_cntx_t;

/* 新增套接字对象 */
typedef struct
{
    int fd;                                 /* 套接字 */
    uint64_t sck_serial;                    /* SCK流水号 */
} srch_add_sck_t;

/* 报头 */
typedef struct
{
    uint32_t type;                          /* 消息类型 */
    size_t length;                          /* 报体长度 */
#define SRCH_MSG_MARK_KEY   (0x1ED23CB4)
    int mark;                               /* 校验值 */
    int flag;                               /* 标识量(0:系统数据类型 1:自定义数据类型) */
    char *body;                             /* 报体 */
} srch_msg_head_t;

/* 报体 */
typedef struct
{
} srch_msg_body_t;



srch_cntx_t *srch_cntx_init(char *pname, const char *conf_path);
void srch_cntx_destroy(srch_cntx_t *ctx);
int srch_getopt(int argc, char **argv, srch_opt_t *opt);
int srch_usage(const char *exec);

int srch_startup(srch_cntx_t *ctx);

log_cycle_t *srch_init_log(char *fname);
int srch_proc_lock(void);
int srch_init_workers(srch_cntx_t *ctx);
int srch_workers_destroy(srch_cntx_t *ctx);
int srch_init_agents(srch_cntx_t *ctx);
int srch_agents_destroy(srch_cntx_t *ctx);

#endif /*__SEARCH_H__*/
