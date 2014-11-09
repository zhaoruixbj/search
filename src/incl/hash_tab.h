#if !defined(__HASH_TAB_H__)
#define __HASH_TAB_H__

#include <pthread.h>

#include "avl_tree.h"

/* 哈希数组 */
typedef struct
{
    int num;                    /* 结点数 */
    avl_tree_t **tree;          /* 平衡二叉树(数组成员: num个) */
    pthread_rwlock_t *lock;     /* 平衡二叉树锁 */

    avl_key_cb_t key_cb;
    avl_cmp_cb_t cmp_cb;
} hash_tab_t;

hash_tab_t *hash_tab_init(int num, avl_key_cb_t key_cb, avl_cmp_cb_t cmp_cb);
int hash_tab_insert(hash_tab_t *hash, void *uk, int uk_len, void *addr);
void *hash_tab_search(hash_tab_t *hash, void *uk, int uk_len);
void *hash_tab_delete(hash_tab_t *hash, void *uk, int uk_len);
int hash_tab_destroy(hash_tab_t *hash);

#endif /*__HASH_TAB_H__*/
