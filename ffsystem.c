/*------------------------------------------------------------------------*/
/* Sample Code of OS Dependent Functions for FatFs                        */
/* (C)ChaN, 2018                                                          */
/*------------------------------------------------------------------------*/

#include "ff.h"
#include <zephyr.h>

//#define HANDLE struct k_sem *

#if _FS_REENTRANT	/* Mutal exclusion */

/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
/  synchronization object for the volume, such as semaphore and mutex.
/  When a 0 is returned, the f_mount() function fails with FR_INT_ERR.
*/

struct k_sem fatfs_sem_arr[2];
size_t fatfs_sem_arr_len = sizeof(fatfs_sem_arr)/sizeof(struct k_sem);

int ff_cre_syncobj (	/* 1:Function succeeded, 0:Could not create the sync object */
	BYTE vol,			/* Corresponding volume (logical drive number) */
	_SYNC_t* sobj		/* Pointer to return the created sync object */
)
{
	/* FreeRTOS */
//	*sobj = xSemaphoreCreateMutex();
//	return (int)(*sobj != NULL);
    vol=0;

    if (vol>=fatfs_sem_arr_len) {
        return 0;
    }

    k_sem_init(&fatfs_sem_arr[vol], 1, 1);
    *sobj=&fatfs_sem_arr[vol];
    return (int)(*sobj != NULL);
}


/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj() function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj (	/* 1:Function succeeded, 0:Could not delete due to an error */
	_SYNC_t sobj		/* Sync object tied to the logical drive to be deleted */
)
{
	/* FreeRTOS */
//  vSemaphoreDelete(sobj);
//	return 1;

    return 1;
}


/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant (	/* 1:Got a grant to access the volume, 0:Could not get a grant */
	_SYNC_t sobj	/* Sync object to wait */
)
{
	/* FreeRTOS */
//	return (int)(xSemaphoreTake(sobj, FF_FS_TIMEOUT) == pdTRUE);

    int res = k_sem_take(sobj, K_MSEC(_FS_TIMEOUT));
    if (res==0) {
        return 1;
    }
    return 0;
}


/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_rel_grant (
	_SYNC_t sobj	/* Sync object to be signaled */
)
{
	/* FreeRTOS */
//	xSemaphoreGive(sobj);

    k_sem_give(sobj);
}

#endif

