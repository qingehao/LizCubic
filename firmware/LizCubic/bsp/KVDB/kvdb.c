#include "rtthread.h"
#include "flashdb.h"
#include "kvdb.h"
rt_mutex_t kvdb_mutex = RT_NULL;
static struct fdb_kvdb kvdb;

static void lock();
static void unlock();

struct _wifiInfo
{
    char ssid[20];
    char password[20];
    uint8_t valid;
};

struct _wifiInfo wifiInfo[10];

static struct fdb_default_kv_node default_kv_set[] =
{
    {"liz_role", "heart"},   // 扮演的角色 分为heart 和 beat
    {"wifiInfo", wifiInfo, sizeof(wifiInfo)},
    {"version", "1.0.0"},
};

int kvdb_init()
{
    int ret = 0;
    struct fdb_blob blob;
    struct fdb_default_kv default_kv;
    default_kv.kvs = default_kv_set;
    default_kv.num = sizeof(default_kv_set) / sizeof(default_kv_set[0]);
    kvdb_mutex = rt_mutex_create("kvdb", RT_IPC_FLAG_FIFO);
    if(kvdb_mutex==NULL)
    {
        return -1;
    }
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
    ret = fdb_kvdb_init(&kvdb, "liz", "fdb_kvdb", &default_kv, NULL);
    if (ret != FDB_NO_ERR) 
    {
        return -1;
    }
    return 0;
}

static void lock()
{
    rt_mutex_take(kvdb_mutex, RT_WAITING_FOREVER);
}

static void unlock()
{
    rt_mutex_release(kvdb_mutex);
}

void kvdb_set_data(char *key, char *value)
{
    fdb_kv_set(&kvdb, key, value);
}

int kvdb_get_data(char *key, char *value)
{
    char *result = NULL;
    result = fdb_kv_get(&kvdb, key);
    if(result != NULL)
    {
        strncpy(value, result, strlen(result)+1);
        return 0;
    }
    else
    {
        return 1;
    }
}

void kvdb_set_blob(char *key, void *value, int len)
{
    struct fdb_blob blob;
    fdb_kv_set_blob(&kvdb, key, fdb_blob_make(&blob, value, len));
}

int kvdb_get_blob(char *key, void *value, int len)
{
    struct fdb_blob blob;
    fdb_kv_get_blob(&kvdb, key, fdb_blob_make(&blob, value, len));
    if(blob.saved.len > 0)
    {
        return 0;
    }
    return 1;
}


