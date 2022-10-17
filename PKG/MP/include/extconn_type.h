#ifndef __EXTCONN_TYPE_H__
#define	__EXTCONN_TYPE_H__

#define EXTCONN_NAME_LEN	16
#define	MAX_EXTCONN_TYPE	20			// External Interface Type
#define MAX_CONN_NUM		50			// External Interface Number

// TPS & STATISTICS  INDEX
enum extconn_type {
	EXTCONN_TYPE_RADIUS	= 0,
	EXTCONN_TYPE_ROME	= 1,
	EXTCONN_TYPE_NSTEP	= 2,
	EXTCONN_TYPE_GTPP   = 3,
	EXTCONN_TYPE_PGW    = 4,
	EXTCONN_TYPE_DIAM   = 5, 	// 2016.07.06
	EXTCONN_TYPE_NTP    = 6,
	/*
	EXTCONN_TYPE_NSCP   = 7,	// 2017.03.08 bhj add NSCP
	EXTCONN_TYPE_MG 	= 8,	// added 2018.01.02 MG
	EXTCONN_TYPE_OFCS 	= 9,	// 2019.12 for OFCS
	*/
	EXTCONN_TYPE_TOTAL	= 7
};

#endif /* __EXTCONN_TYPE_H__ */
