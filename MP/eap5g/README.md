

Number 	Name 	ESP Reference 	IKEv2 Reference 
0	Reserved	[RFC7296]	-
1	ENCR_DES_IV64	UNSPECIFIED	-
2	ENCR_DES	[RFC2405]	[RFC7296]
3	ENCR_3DES	[RFC2451]	[RFC7296]
4	ENCR_RC5	[RFC2451]	[RFC7296]
5	ENCR_IDEA	[RFC2451]	[RFC7296]
6	ENCR_CAST	[RFC2451]	[RFC7296]
7	ENCR_BLOWFISH	[RFC2451]	[RFC7296]
8	ENCR_3IDEA	UNSPECIFIED	[RFC7296]
9	ENCR_DES_IV32	UNSPECIFIED	-
10	Reserved	[RFC7296]	-
11	ENCR_NULL	[RFC2410]	Not allowed
12	ENCR_AES_CBC	[RFC3602]	[RFC7296]
13	ENCR_AES_CTR	[RFC3686]	[RFC5930]
14	ENCR_AES_CCM_8	[RFC4309]	[RFC5282]
15	ENCR_AES_CCM_12	[RFC4309]	[RFC5282]
16	ENCR_AES_CCM_16	[RFC4309]	[RFC5282]
17	Unassigned		
18	ENCR_AES_GCM_8	[RFC4106] [RFC8247]	[RFC5282] [RFC8247]
19	ENCR_AES_GCM_12	[RFC4106] [RFC8247]	[RFC5282] [RFC8247]
20	ENCR_AES_GCM_16	[RFC4106] [RFC8247]	[RFC5282] [RFC8247]
21	ENCR_NULL_AUTH_AES_GMAC	[RFC4543]	Not allowed
22	Reserved for IEEE P1619 XTS-AES	[Matt_Ball]	-
23	ENCR_CAMELLIA_CBC	[RFC5529]	[RFC7296]
24	ENCR_CAMELLIA_CTR	[RFC5529]	-
25	ENCR_CAMELLIA_CCM_8	[RFC5529] [RFC8247]	-
26	ENCR_CAMELLIA_CCM_12	[RFC5529] [RFC8247]	-
27	ENCR_CAMELLIA_CCM_16	[RFC5529] [RFC8247]	-
28	ENCR_CHACHA20_POLY1305	[RFC7634]	[RFC7634]
29	ENCR_AES_CCM_8_IIV	[RFC8750]	Not allowed
30	ENCR_AES_GCM_16_IIV	[RFC8750]	Not allowed
31	ENCR_CHACHA20_POLY1305_IIV	[RFC8750]	Not allowed
32	ENCR_KUZNYECHIK_MGM_KTREE	[RFC9227]	[RFC9227]
33	ENCR_MAGMA_MGM_KTREE	[RFC9227]	[RFC9227]
34	ENCR_KUZNYECHIK_MGM_MAC_KTREE	[RFC9227]	Not allowed
35	ENCR_MAGMA_MGM_MAC_KTREE	[RFC9227]	Not allowed
36-1023	Unassigned		
1024-65535	Private use	[RFC7296]	[RFC7296]

Number 	Name 	Reference 
0	NONE	[RFC7296]
1	AUTH_HMAC_MD5_96	[RFC2403][RFC7296]
2	AUTH_HMAC_SHA1_96	[RFC2404][RFC7296]
3	AUTH_DES_MAC	[UNSPECIFIED]
4	AUTH_KPDK_MD5	[UNSPECIFIED]
5	AUTH_AES_XCBC_96	[RFC3566][RFC7296]
6	AUTH_HMAC_MD5_128	[RFC4595]
7	AUTH_HMAC_SHA1_160	[RFC4595]
8	AUTH_AES_CMAC_96	[RFC4494]
9	AUTH_AES_128_GMAC	[RFC4543]
10	AUTH_AES_192_GMAC	[RFC4543]
11	AUTH_AES_256_GMAC	[RFC4543]
12	AUTH_HMAC_SHA2_256_128	[RFC4868]
13	AUTH_HMAC_SHA2_384_192	[RFC4868]
14	AUTH_HMAC_SHA2_512_256	[RFC4868]
15-1023	Unassigned	
1024-65535	Private use	[RFC7296]