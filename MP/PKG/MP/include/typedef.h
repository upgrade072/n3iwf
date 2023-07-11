#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include <stdint.h>

#ifndef _INCLUDE_HPUX_SOURCE
typedef unsigned short  ushort;
typedef unsigned int    uint;
#endif
typedef unsigned char   uchar;

#ifndef bool
typedef unsigned char	bool;
#endif

#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#ifndef ON
#define ON		1
#endif

#ifndef OFF
#define OFF		0
#endif

typedef struct ip_info {
	uint8_t		af;	/* AF_INET or AF_INET6 */

#define			MAX_IPv6_ADDR	16
#define			MAX_IPv4_ADDR	4
	uint8_t		ip_addr[MAX_IPv6_ADDR];
	uint16_t	port;
} ip_info_t;


#endif /*__TYPEDEF_H__ */

