#ifndef __COMM_SHM_H__
#define __COMM_SHM_H__

#include "conflib.h"
#include "typedef.h"
 
#define SHM_CREATE      1
#define SHM_EXIST       2
#define SHM_GET			3
#define READ_MODE		0x04
#define WRITE_MODE		0x02
 
extern long commlib_shm_get_key (char *file, char *name);
extern  int commlib_getShmKey(char *l_sysconf, char *shmName, long *key);
extern  int commlib_crteShm(char *l_sysconf, char *shmName, size_t size, void **shmPtr);
extern  int commlib_attachShm(long key, size_t size, void **shmPtr);
extern int commlib_attachShm2(long key, size_t size, void **shmPtr);
extern int commlib_crteShm2(char *l_sysconf, char *shmName, size_t size, void **shmPtr);
extern  int commlib_getShm(char *l_sysconf, char *shmName, size_t size, void **shmPtr);
extern	int GetShmKey(char *l_sysconf, char *shmName, long *key);
extern int _readShmPtr(long key, unsigned long *shmAddr);
//extern long commlib_shm_get_key (char *file, char *name);
extern int commlib_shm_get (char *name, size_t size, void **shm);
extern int commlib_shm_attach (char *name, size_t size, void **shm);

 
#endif /*__COMM_SHM_H__*/
