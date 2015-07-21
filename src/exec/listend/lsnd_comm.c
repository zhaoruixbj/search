#include "mesg.h"
#include "listend.h"

/******************************************************************************
 **函数名称: lsnd_getopt 
 **功    能: 解析输入参数
 **输入参数: 
 **     argc: 参数个数
 **     argv: 参数列表
 **输出参数:
 **     opt: 参数选项
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 解析输入参数
 **     2. 验证输入参数
 **注意事项: 
 **     N: 服务名 - 根据服务名可找到配置路径
 **     h: 帮助手册
 **作    者: # Qifeng.zou # 2014.11.15 #
 ******************************************************************************/
int lsnd_getopt(int argc, char **argv, lsnd_opt_t *opt)
{
    int ch;

    /* 1. 解析输入参数 */
    while (-1 != (ch = getopt(argc, argv, "N:hd")))
    {
        switch (ch)
        {
            case 'N':   /* 结点名 */
            {
                snprintf(opt->name, sizeof(opt->name), "%s", optarg);
                break;
            }
            case 'd':
            {
                opt->isdaemon = true;
                break;
            }
            case 'h':   /* 显示帮助信息 */
            default:
            {
                return LSND_SHOW_HELP;
            }
        }
    }

    optarg = NULL;
    optind = 1;

    /* 2. 验证输入参数 */
    if (!strlen(opt->name))
    {
        return LSND_SHOW_HELP;
    }

    return 0;
}

/* 显示启动参数帮助信息 */
int lsnd_usage(const char *exec)
{
    printf("\nUsage: %s [-h] [-d] -c <config file>\n", exec);
    printf("\t-h\tShow help\n"
           "\t-c\tConfiguration path\n\n");
    return 0;
}

/* 初始化日志模块 */
log_cycle_t *lsnd_init_log(char *fname)
{
    char path[FILE_NAME_MAX_LEN];

    log_get_path(path, sizeof(path), basename(fname));

    return log_init(LOG_LEVEL_ERROR, path);
}

/******************************************************************************
 **函数名称: lsnd_attach_distq
 **功    能: 附着分发队列
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015-07-05 18:12:16 #
 ******************************************************************************/
int lsnd_attach_distq(lsnd_cntx_t *ctx)
{
    int idx;
    char path[FILE_NAME_MAX_LEN];
    lsnd_conf_t *conf = &ctx->conf;

    /* > 申请对象空间 */
    ctx->distq = (shm_queue_t **)calloc(1, conf->distq.num*sizeof(shm_queue_t *));
    if (NULL == ctx->distq)
    {
        log_error(ctx->log, "Alloc memory from slab failed!");
        return LSND_ERR;
    }

    /* > 依次附着队列 */
    for (idx=0; idx<conf->distq.num; ++idx)
    {
        LSND_GET_DISTQ_PATH(path, sizeof(path), conf->wdir, idx);

        ctx->distq[idx] = shm_queue_attach(path);
        if (NULL == ctx->distq[idx])
        {
            free(ctx->distq);
            log_error(ctx->log, "errmsg:[%d] %s! path:%s", errno, strerror(errno), path);
            return LSND_ERR;
        }
    }

    return LSND_OK;
}
