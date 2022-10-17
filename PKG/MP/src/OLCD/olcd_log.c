
#include "olcd_proto.h"

int initLog(char *appName)
{
	LoglibProperty  property;

	if( loglib_initLog(appName) < 0 ) {
		fprintf (stderr,"[%s, %d] loglib_initLog failed\n", FL);
		return -1;
	}

	memset(&property, 0, sizeof(property));

	strcpy(property.appName, appName);
	property.num_suffix = 0;
	//property.limit_val  = 0;
	property.limit_val  = 1024*1024*30;
	property.mode       = LOGLIB_MODE_LIMIT_SIZE |
					      LOGLIB_FLUSH_IMMEDIATE |
		 				  LOGLIB_MODE_DAILY |
						  LOGLIB_TIME_STAMP_FIRST |
						  LOGLIB_FNAME_LNUM;

	strcpy(property.subdir_format, "%Y-%m-%d");
	sprintf (property.fname, "%s/log/ERR_LOG/OLCD/%s.%s", homeEnv, mySysName, appName);
	if ((trcErrLogId=loglib_openLog(&property)) < 0 ) {
		fprintf (stderr, "[%s.%d] loglib_openLog() fail[%s]\n", FL, property.fname);
		return -1;
	}

	property.mode       = 0;
	property.mode       = LOGLIB_MODE_LIMIT_SIZE |
						  LOGLIB_FLUSH_IMMEDIATE |
						  LOGLIB_MODE_DAILY |
						  LOGLIB_TIME_STAMP_FIRST; 

	sprintf (property.fname, "%s/%s", homeEnv, OVLD_MONITORING_LOG);
	if ( olcd_makeDirPath(property.fname) < 0 ) {
		fprintf (stderr, "[%s.%d] olcd_makeDirPath() fail[%s]\n", FL, property.fname);
		return -1;
	}

	if ((ovldMonLog=loglib_openLog(&property)) < 0 ) {
		fprintf (stderr, "[%s.%d] loglib_openLog() fail[%s]\n", FL, property.fname);
		return -1;
	}

	return 0;
}

