#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "fal.h"
#include <dfs_fs.h>
#include "lizCubic.h"
#define DBG_TAG    "main"
#define DBG_LVL    DBG_LOG
#include <rtdbg.h>

#define FS_PARTITION_NAME              "fs"

int main(void)
{
    struct rt_device *mtd_dev = RT_NULL;

    fal_init();
    mtd_dev = fal_mtd_nor_device_create(FS_PARTITION_NAME);
    if (!mtd_dev)
    {
        LOG_E("Can't create a mtd device on '%s' partition.", FS_PARTITION_NAME);
    }
    else
    {
        if (dfs_mount(FS_PARTITION_NAME, "/", "lfs", 0, 0) == 0)
        {
            LOG_I("Filesystem initialized!");
        }
        else
        {
            dfs_mkfs("lfs", FS_PARTITION_NAME);
            if (dfs_mount("filesystem", "/", "lfs", 0, 0) == 0)
            {
                LOG_I("Filesystem initialized!");
            }
            else
            {
                LOG_E("Failed to initialize filesystem!");
            }
        }
    }   
    liz_app_init();
}
void pwm_test()
{
    
}