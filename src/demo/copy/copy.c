/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: copy.c
 ** 版本号: 1.0
 ** 描  述: 
 ** 作  者: # Qifeng.zou # Mon 27 Jul 2015 07:08:59 PM CST #
 ******************************************************************************/

#include "comm.h"
#include "redo.h"
#include "thread_pool.h"

#define CP_THD_NUM      (4)         /* 线程数 */
#define CP_SLOT_SIZE    (4 * MB)    /* 每次拷贝大小 */

typedef struct
{
    thread_pool_t *tpool;           /* 线程池 */

    char src[FILE_PATH_MAX_LEN];    /* 源路径 */
    char dst[FILE_PATH_MAX_LEN];    /* 目的路径 */

    struct stat fst;                /* 源文件大小 */
} cp_cntx_t;

void *cp_copy_routine(void *_ctx)
{
    int tid, fd, to;
    ssize_t n, size;
    off_t off;
    char *buff;
    cp_cntx_t *ctx = (cp_cntx_t *)_ctx;

    tid = thread_pool_get_tidx(ctx->tpool);

    buff = (char *)calloc(1, CP_SLOT_SIZE);
    if (NULL == buff)
    {
        fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
        pthread_exit((void *)-1);
        return (void *)-1;
    }

    fd = Open(ctx->src, O_RDONLY, OPEN_MODE);
    if (fd < 0)
    {
        fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
        pthread_exit((void *)-1);
        return (void *)-1;
    }

    to = Open(ctx->dst, O_CREAT|O_WRONLY, OPEN_MODE);
    if (to < 0)
    {
        fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
        pthread_exit((void *)-1);
        return (void *)-1;
    }

    off = tid * CP_SLOT_SIZE;
    while (1)
    {
        if (off > ctx->fst.st_size)
        {
            break;
        }
        else if (off + CP_SLOT_SIZE > ctx->fst.st_size)
        {
            size = ctx->fst.st_size - off;
        }
        else
        {
            size = CP_SLOT_SIZE;
        }

        n = Readn(fd, buff, size);
        if (n != size)
        {
            fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
            break;
        }

        n = Writen(to, buff, size);
        if (n != size)
        {
            fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
            break;
        }

        off += (ctx->tpool->num * CP_SLOT_SIZE);
    }

    CLOSE(fd);
    CLOSE(to);
    pthread_exit((void *)-1);

    return (void *)0;
}

int main(int argc, char *argv[])
{
    int idx;
    cp_cntx_t *ctx;
    thread_pool_opt_t opt;

    if (3 != argc)
    {
        fprintf(stderr, "Paramter isn't right!\n");
        return -1;
    }

    /* > 初始化处理 */
    ctx = (cp_cntx_t *)calloc(1, sizeof(cp_cntx_t));
    if (NULL == ctx)
    {
        fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
        return -1;
    }

    snprintf(ctx->src, sizeof(ctx->src), "%s", argv[1]);
    snprintf(ctx->dst, sizeof(ctx->dst), "%s", argv[2]);

    if (stat(ctx->src, &ctx->fst))
    {
        fprintf(stderr, "errmsg:[%d] %s! path:%s\n", errno, strerror(errno), ctx->src);
        return -1;
    }

    /* > 创建内存池 */
    opt.pool = (void *)NULL;
    opt.alloc = (mem_alloc_cb_t)mem_alloc;
    opt.dealloc = (mem_dealloc_cb_t)mem_dealloc;

    ctx->tpool = thread_pool_init(CP_THD_NUM, &opt, NULL);
    if (NULL == ctx->tpool)
    {
        fprintf(stderr, "errmsg:[%d] %s!\n", errno, strerror(errno));
        return -1;
    }

    /* > 执行拷贝处理 */
    for (idx=0; idx<CP_THD_NUM; ++idx)
    {
        thread_pool_add_worker(ctx->tpool, cp_copy_routine, ctx);
    }

    while (1) { pause(); }

    return 0;
}