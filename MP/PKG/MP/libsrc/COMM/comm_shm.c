#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <comm_shm.h> 

int _readShmPtr(long key, unsigned long *shmAddr);
int _writeShmPtr(long key, void* shmPtr);

int commlib_crteShm(char *l_sysconf, char *shmName, size_t size, void **shmPtr)
{
	long   key;

	if( commlib_getShmKey(l_sysconf, shmName, &key)<0 ) {
		fprintf (stderr, "Not Found %s's SHARED_MEMORY_KEY Key\n", shmName);
		return -1;
	}

	return commlib_attachShm(key, size, shmPtr);
}

/* 고정 shmptr 을 사용하기 위하여 별도로 만듦 */
int commlib_crteShm2(char *l_sysconf, char *shmName, size_t size, void **shmPtr)
{
	long   key;

	if( commlib_getShmKey(l_sysconf, shmName, &key)<0 ) {
		fprintf (stderr, "Not Found %s's SHARED_MEMORY_KEY Key\n", shmName);
		return -1;
	}

	return commlib_attachShm2(key, size, shmPtr);
}

int commlib_attachShm(long key, size_t size, void **shmPtr)
{
	int   shmId;
	unsigned long shmAddr;

	shmId = shmget(key, size, IPC_CREAT|IPC_EXCL|0644);
	if (shmId < 0) {
		if (errno == EEXIST) {
			shmId = shmget(key, size, 0);
			if (shmId < 0) {
#if 0
				printf ("[%s] EXIST, shmget: key=0x%lx size=%u errno=%d(%s)\n",
						__func__, key, size, errno, strerror(errno));
#endif
				return -1;
			}
			*shmPtr = (void *)shmat(shmId, 0, 0);
			if (*shmPtr == (void *)-1) {
#if 0
				printf ("[%s] EXIST, shmat: key=0x%lx size=%u errno=%d(%s)\n",
						__func__, key, size, errno, strerror(errno));
#endif
				return -1;
			}
#if 1
			printf ("[%s] EXIST, shmat: key=0x%lx size=%lu *shmPtr=%p\n",
					__func__, key, size, *shmPtr);
#endif
			return SHM_EXIST;	
		} else {
#if 0
			printf ("[%s] ERROR, shmget: key=0x%lx size=%u errno=%d(%s)\n",
					__func__, key, size, errno, strerror(errno));
#endif
			return -1;
		}
	}

 	*shmPtr = (void *)shmat(shmId, 0, 0);
    if (*shmPtr == (void *)-1) {
		printf ("[%s] CREATE, shmat: key=0x%lx size=%lu errno=%d(%s)\n",
				__func__, key, (unsigned long)size, errno, strerror(errno));
        return -1;
    }
    memset (*shmPtr, 0x0, size);

#if 1
    printf ("[%s] CREATE, shmget: key=0x%lx size=%lu *shmPtr=%p\n",
			__func__, key, size, *shmPtr);
#endif
    return SHM_CREATE;
}

int commlib_attachShm2(long key, size_t size, void **shmPtr)
{
	int   shmId;
	unsigned long shmAddr;

	shmId = shmget(key, size, IPC_CREAT|IPC_EXCL|0644);
	if (shmId < 0) {
		if (errno == EEXIST) {
			shmId = shmget(key, size, 0);
			if (shmId < 0) {
				printf ("[%s] EXIST, shmget: key=0x%lx size=%lu errno=%d(%s)\n",
						__func__, key, (unsigned long)size, errno, strerror(errno));
				return -1;
			}
			if(_readShmPtr(key, &shmAddr) < 0)
				return -1;
			*shmPtr = (void *)shmat(shmId, (void*)shmAddr, SHM_REMAP);
			if (*shmPtr == (void *)-1) {
				printf ("[%s] EXIST, shmat: key=0x%lx size=%lu errno=%d(%s)\n",
						__func__, key, (unsigned long)size, errno, strerror(errno));
				return -1;
			}
			return SHM_EXIST;	
		} else {
			printf ("[%s] ERROR, shmget: key=0x%lx size=%lu errno=%d(%s)\n",
					__func__, key, (unsigned long)size, errno, strerror(errno));
			return -1;
		}
	}

 	*shmPtr = (void *)shmat(shmId, 0, 0);
    if (*shmPtr == (void *)-1) {
		printf ("[%s] CREATE, shmat: key=0x%lx size=%lu errno=%d(%s)\n",
				__func__, key, (unsigned long)size, errno, strerror(errno));
        return -1;
    }
    memset (*shmPtr, 0x0, size);
	_writeShmPtr(key, *shmPtr);

//    printf ("[%s] CREATE, shmget: key=0x%lx size=%lu *shmPtr=%p\n",
//			__func__, key, size, *shmPtr);
    return SHM_CREATE;
}


int commlib_getShmKey(char *l_sysconf, char *shmName, long *key)
{
    char tmp[64];

    if( conflib_getNthTokenInFileSection (l_sysconf, "[SHARED_MEMORY_KEY]", shmName, 1, tmp)<0 )
        return -1;

    //*key = (int )strtol(tmp, 0, 0);
    *key = strtol(tmp, 0, 0);
	return 0;
}

long commlib_shm_get_key (char *file, char *name)
{
	long key = 0;;
	const char *sysconf = file ? file : conflib_get_sysconf (); 

	if (sysconf == NULL || sysconf[0] == 0x00)
	{
		fprintf (stderr, "[%s.%d] file:%p (%s)\n", __FILE__, __LINE__, sysconf, sysconf ? sysconf : "-");
		return -1;
	}

	if (commlib_getShmKey ((char *)sysconf, name, &key) < 0)
		return -1;

	return key;
}

int
commlib_shm_get (char *name, size_t size, void **shm)
{
	long key = commlib_shm_get_key (NULL, name);
	if (key < 0)
		return -1;

	return commlib_attachShm(key, size, shm);
}

int commlib_getShm(char *sysconf, char *shmName, size_t size, void **shmPtr)
{
 
    long	key;
	int		shmid;
 
    if(commlib_getShmKey (sysconf, shmName, &key)<0 ) {
        fprintf (stderr, "Not Found %s's SHARED_MEMORY_KEY Key\n", shmName);
        return -1;
    }

	shmid = shmget(key, size, IPC_EXCL); 

	if (shmid < 0) {
		fprintf (stderr, "Fail shmget shmName=%s key=0x%lx errno=%d size=%lu\n", 
            shmName, key, errno, (unsigned long)size);
		return -1;
	}

	if ((*shmPtr = (void *)shmat(shmid, 0, 0)) == (void *)(-1)) {
        fprintf (stderr,"Fail shmgat shmName=%s key=0x%lx errno=%d size=%lu\n", 
            shmName, key, errno, (unsigned long)size);
        return -1;
    }

    return 1;
}

int commlib_shm_attach (char *name, size_t size, void **shm_pp)
{
	char *sysconf = conflib_get_sysconf ();
	if (sysconf)
		return commlib_getShm (sysconf, name, size, shm_pp);

	return -1;
}

int _writeShmPtr(long key, void* shmPtr)
{
	FILE *fp;
	char fname[64], buf[32];
	size_t n;

	sprintf(fname, "%s/bin/.shm_key_0x%lx", homeEnv, key);
	sprintf(buf, "%p", shmPtr);

	fp = fopen(fname, "wt");
	if(fp == NULL) {
		printf("File open error. fname=[%s]\n", fname);
		return -1;
	}

	n = fwrite(buf, sizeof(char), strlen(buf), fp);
	if(n != strlen(buf)) {
		printf("File write error. fname=[%s] data=[%s]\n", fname, buf);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

int _readShmPtr(long key, unsigned long *shmAddr)
{
	FILE *fp;
	char fname[64], buf[32];
	int n;

	sprintf(fname, "%s/bin/.shm_key_0x%lx", homeEnv, key);

	fp = fopen(fname, "rt");
	if(fp == NULL) {
		printf("File open error. fname=[%s]\n", fname);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	n = fread(buf, sizeof(char), sizeof(buf), fp);

	*shmAddr = strtol(buf, NULL, 16);

	fclose(fp);
	return 0;
}
