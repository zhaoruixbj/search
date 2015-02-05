/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: sdtp.c
 ** 版本号: 1.0
 ** 描  述: 共享消息传输通道(Sharing Message Transaction Channel)
 **         1. 主要用于异步系统之间数据消息的传输
 ** 作  者: # Qifeng.zou # 2014.12.29 #
 ******************************************************************************/
#include <memory.h>
#include <assert.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "sdtp.h"
#include "syscall.h"
#include "xml_tree.h"
#include "sdtp_cmd.h"
#include "sdtp_priv.h"
#include "thread_pool.h"

static int _sdtp_init(sdtp_cntx_t *ctx);
static int sdtp_creat_recvq(sdtp_cntx_t *ctx);

static int sdtp_creat_recvtp(sdtp_cntx_t *ctx);
void sdtp_recvtp_destroy(void *_ctx, void *args);

static int sdtp_creat_worktp(sdtp_cntx_t *ctx);
void sdtp_worktp_destroy(void *_ctx, void *args);

static int sdtp_proc_def_hdl(uint32_t type, char *buff, size_t len, void *args);

/******************************************************************************
 **函数名称: sdtp_init
 **功    能: 初始化SDTP接收端
 **输入参数: 
 **     conf: 配置信息
 **     log: 日志对象
 **输出参数: NONE
 **返    回: 全局对象
 **实现描述: 
 **     1. 创建全局对象
 **     2. 备份配置信息
 **     3. 初始化接收端
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
sdtp_cntx_t *sdtp_init(const sdtp_conf_t *conf, log_cycle_t *log)
{
    sdtp_cntx_t *ctx;

    /* 1. 创建全局对象 */
    ctx = (sdtp_cntx_t *)calloc(1, sizeof(sdtp_cntx_t));
    if (NULL == ctx)
    {
        printf("errmsg:[%d] %s!", errno, strerror(errno));
        return NULL;
    }

    ctx->log = log;

    /* 2. 备份配置信息 */
    memcpy(&ctx->conf, conf, sizeof(sdtp_conf_t));

    ctx->conf.rqnum = SDTP_WORKER_HDL_QNUM * conf->work_thd_num;

    /* 3. 初始化接收端 */
    if (_sdtp_init(ctx))
    {
        Free(ctx);
        log_error(ctx->log, "Initialize recv failed!");
        return NULL;
    }

    return ctx;
}

/******************************************************************************
 **函数名称: sdtp_startup
 **功    能: 启动SDTP接收端
 **输入参数: 
 **     conf: 配置信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 设置接收线程回调
 **     2. 设置工作线程回调
 **     3. 创建侦听线程
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
int sdtp_startup(sdtp_cntx_t *ctx)
{
    int idx;
    thread_pool_t *tp;
    sdtp_lsn_t *lsn = &ctx->listen;

    /* 1. 设置接收线程回调 */
    tp = ctx->recvtp;
    for (idx=0; idx<tp->num; ++idx)
    {
        thread_pool_add_worker(tp, sdtp_rsvr_routine, ctx);
    }

    /* 2. 设置工作线程回调 */
    tp = ctx->worktp;
    for (idx=0; idx<tp->num; ++idx)
    {
        thread_pool_add_worker(tp, sdtp_worker_routine, ctx);
    }
    
    /* 3. 创建侦听线程 */
    if (thread_creat(&lsn->tid, sdtp_listen_routine, ctx))
    {
        log_error(ctx->log, "Start listen failed");
        return SDTP_ERR;
    }
    
    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_register
 **功    能: 消息处理的注册接口
 **输入参数: 
 **     ctx: 全局对象
 **     type: 扩展消息类型 Range:(0 ~ SDTP_TYPE_MAX)
 **     proc: 回调函数
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **     1. 只能用于注册处理扩展数据类型的处理
 **     2. 不允许重复注册 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
int sdtp_register(sdtp_cntx_t *ctx, uint32_t type, sdtp_reg_cb_t proc, void *args)
{
    sdtp_reg_t *reg;

    if (type >= SDTP_TYPE_MAX)
    {
        log_error(ctx->log, "Data type is out of range!");
        return SDTP_ERR;
    }

    if (0 != ctx->reg[type].flag)
    {
        log_error(ctx->log, "Repeat register type [%d]!", type);
        return SDTP_ERR_REPEAT_REG;
    }

    reg = &ctx->reg[type];
    reg->type = type;
    reg->proc = proc;
    reg->args = args;
    reg->flag = 1;

    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_destroy
 **功    能: 销毁SDTP对象
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
int sdtp_destroy(sdtp_cntx_t **ctx)
{
    /* 1. 销毁侦听线程 */
    sdtp_listen_destroy(&(*ctx)->listen);

#if 0
    /* 2. 销毁接收线程池 */
    thread_pool_destroy_ext((*ctx)->recvtp, sdtp_recvtp_destroy, *ctx);

    /* 3. 销毁工作线程池 */
    thread_pool_destroy_ext((*ctx)->worktp, sdtp_worktp_destroy, *ctx);
#endif

    Free(*ctx);
    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_reg_init
 **功    能: 初始化注册对象
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
static int sdtp_reg_init(sdtp_cntx_t *ctx)
{
    int idx;
    sdtp_reg_t *reg = &ctx->reg[0];

    for (idx=0; idx<SDTP_TYPE_MAX; ++idx, ++reg)
    {
        reg->type = idx;
        reg->proc = sdtp_proc_def_hdl;
        reg->flag = 0;
        reg->args = NULL;
    }
    
    return SDTP_OK;
}

/******************************************************************************
 **函数名称: _sdtp_init
 **功    能: 初始化接收对象
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 初始化注册信息
 **     2. 创建接收队列
 **     3. 创建接收线程池
 **     4. 创建工作线程池
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
static int _sdtp_init(sdtp_cntx_t *ctx)
{
    /* 1. 初始化注册信息 */
    sdtp_reg_init(ctx);

    /* 2. 创建接收队列 */
    if (sdtp_creat_recvq(ctx))
    {
        log_error(ctx->log, "Create recv queue failed!");
        return SDTP_ERR;
    }

    /* 3. 创建接收线程池 */
    if (sdtp_creat_recvtp(ctx))
    {
        log_error(ctx->log, "Create recv thread pool failed!");
        return SDTP_ERR;
    }

    /* 4. 创建工作线程池 */
    if (sdtp_creat_worktp(ctx))
    {
        log_error(ctx->log, "Create worker thread pool failed!");
        return SDTP_ERR;
    }

    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_creat_recvq
 **功    能: 创建接收队列
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 创建队列数组
 **     2. 依次创建接收队列
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.30 #
 ******************************************************************************/
static int sdtp_creat_recvq(sdtp_cntx_t *ctx)
{
    int idx;
    sdtp_conf_t *conf = &ctx->conf;

    /* 1. 创建队列数组 */
    ctx->recvq = calloc(conf->rqnum, sizeof(queue_t *));
    if (NULL == ctx->recvq)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return SDTP_ERR;
    }

    /* 2. 依次创建接收队列 */
    for(idx=0; idx<conf->rqnum; ++idx)
    {
        ctx->recvq[idx] = queue_creat(conf->recvq.max, conf->recvq.size);
        if (NULL == ctx->recvq[idx])
        {
            log_error(ctx->log, "Create queue failed! max:%d size:%d",
                    conf->recvq.max, conf->recvq.size);
            return SDTP_ERR;
        }
    }

    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_creat_recvtp
 **功    能: 创建接收线程池
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 创建线程池
 **     2. 创建接收对象
 **     3. 初始化接收对象
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.01 #
 ******************************************************************************/
static int sdtp_creat_recvtp(sdtp_cntx_t *ctx)
{
    int idx;
    sdtp_rsvr_t *rsvr;
    sdtp_conf_t *conf = &ctx->conf;

    /* 1. 创建线程池 */
    ctx->recvtp = thread_pool_init(conf->recv_thd_num, 4*KB);
    if (NULL == ctx->recvtp)
    {
        log_error(ctx->log, "Initialize thread pool failed!");
        return SDTP_ERR;
    }

    /* 2. 创建接收对象 */
    ctx->recvtp->data = (void *)calloc(conf->recv_thd_num, sizeof(sdtp_rsvr_t));
    if (NULL == ctx->recvtp->data)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));

        thread_pool_destroy(ctx->recvtp);
        ctx->recvtp = NULL;
        return SDTP_ERR;
    }

    /* 3. 初始化接收对象 */
    rsvr = (sdtp_rsvr_t *)ctx->recvtp->data;
    for (idx=0; idx<conf->recv_thd_num; ++idx, ++rsvr)
    {
        if (sdtp_rsvr_init(ctx, rsvr, idx))
        {
            log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));

            thread_pool_destroy(ctx->recvtp);
            ctx->recvtp = NULL;

            return SDTP_ERR;
        }
    }

    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_recvtp_destroy
 **功    能: 销毁接收线程池
 **输入参数: 
 **     ctx: 全局对象
 **     args: 附加参数
 **输出参数: NONE
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.01 #
 ******************************************************************************/
void sdtp_recvtp_destroy(void *_ctx, void *args)
{
    int idx;
    sdtp_cntx_t *ctx = (sdtp_cntx_t *)_ctx;
    sdtp_rsvr_t *rsvr = (sdtp_rsvr_t *)ctx->recvtp->data;

    for (idx=0; idx<ctx->conf.recv_thd_num; ++idx, ++rsvr)
    {
        /* 1. 关闭命令套接字 */
        Close(rsvr->cmd_sck_id);

        /* 2. 关闭通信套接字 */
        sdtp_rsvr_del_all_conn_hdl(rsvr);

        slab_destroy(rsvr->pool);
    }

    Free(ctx->recvtp->data);
    thread_pool_destroy(ctx->recvtp);

    return;
}

/******************************************************************************
 **函数名称: sdtp_creat_worktp
 **功    能: 创建工作线程池
 **输入参数: 
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 创建线程池
 **     2. 创建工作对象
 **     3. 初始化工作对象
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
static int sdtp_creat_worktp(sdtp_cntx_t *ctx)
{
    int idx;
    sdtp_worker_t *worker;
    sdtp_conf_t *conf = &ctx->conf;

    /* 1. 创建线程池 */
    ctx->worktp = thread_pool_init(conf->work_thd_num, 4*KB);
    if (NULL == ctx->worktp)
    {
        log_error(ctx->log, "Initialize thread pool failed!");
        return SDTP_ERR;
    }

    /* 2. 创建工作对象 */
    ctx->worktp->data = (void *)calloc(conf->work_thd_num, sizeof(sdtp_worker_t));
    if (NULL == ctx->worktp->data)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));

        thread_pool_destroy(ctx->worktp);
        ctx->worktp = NULL;
        return SDTP_ERR;
    }

    /* 3. 初始化工作对象 */
    worker = ctx->worktp->data;
    for (idx=0; idx<conf->work_thd_num; ++idx, ++worker)
    {
        if (sdtp_worker_init(ctx, worker, idx))
        {
            log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));

            thread_pool_destroy(ctx->recvtp);
            ctx->recvtp = NULL;

            return SDTP_ERR;
        }
    }
    
    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdtp_worktp_destroy
 **功    能: 销毁工作线程池
 **输入参数: 
 **     ctx: 全局对象
 **     args: 附加参数
 **输出参数: NONE
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
void sdtp_worktp_destroy(void *_ctx, void *args)
{
    int idx;
    sdtp_cntx_t *ctx = (sdtp_cntx_t *)_ctx;
    sdtp_conf_t *conf = &ctx->conf;
    sdtp_worker_t *worker = (sdtp_worker_t *)ctx->worktp->data;

    for (idx=0; idx<conf->work_thd_num; ++idx, ++worker)
    {
        Close(worker->cmd_sck_id);
    }

    Free(ctx->worktp->data);
    thread_pool_destroy(ctx->recvtp);

    return;
}

/******************************************************************************
 **函数名称: sdtp_proc_def_hdl
 **功    能: 默认消息处理函数
 **输入参数: 
 **     type: 消息类型
 **     buff: 消息内容
 **     len: 内容长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
static int sdtp_proc_def_hdl(uint32_t type, char *buff, size_t len, void *args)
{
    log2_error("Call %s() type:%d len:%d", __func__, type, len);

    return SDTP_OK;
}