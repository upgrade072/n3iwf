#ifndef __SYSCONF_H__
#define __SYSCONF_H__

/*------------------------------------------------------------------------------
// ��� �ý��ۿ��� �������� ���Ǵ� Configuration�� ���õ� ������ �����Ѵ�.
//------------------------------------------------------------------------------*/

#define	MY_SYS_NAME						"MY_SYS_NAME"
#define IV_HOME							"IV_HOME"
#define	SYS_MP_ID						"SYS_MP_ID"
#define SYSCONF_FILE					"data/sysconfig"
#define ASSOCONF_FILE                   "data/associate_config"
#define DATACONF_FILE					"data/dataconfig"
#define APPCONF_DIR                     "data/CONFIG"
#define EXCONN_CONF_FILE                "data/CONFIG/extconn.conf"
#define DATA_DIR                     	"data"


//#define SYSCONF_MAX_APPL_NUM			64	/* �ý��۴� application process �ִ� ���� */
#define SYSCONF_MAX_APPL_NUM			75	/* �ý��۴� application process �ִ� ���� */
#define SYSCONF_MAX_ASSO_SYS_NUM		32	/* MPA, MPB, OMP	������� �ý��� �ִ� ���� */
#define SYSCONF_MAX_SYS_TYPE_NUM		2	/* MP, OMP			�ý��� type �ִ� ���� */
#define SYSCONF_MAX_SYS_TYPE_MEMBER		9	/* MP->MPA, MPB	type�� �ý��� �ִ� ���� */
#define SYSCONF_MAX_GROUP_NUM			2	/* MP, OMP 			cluster�� ����ȭ�� �ý��� group �ִ� ���� */
#define SYSCONF_MAX_GROUP_MEMBER		9	/* MP->MPA, MPB 	�׷�� �ý��� member �ִ� ���� */

#define SYSCONF_MAX_OMAF_PROC_NUM		5
#define	SYSCONF_MAX_EHTTPF_THREAD		20	/* 2011.12.02. 16->20 */
#define	SYSCONF_MAX_DOMF_THR_NUM		16
#define	SYSCONF_MAX_MGMF_THR_NUM		8
#define	SYSCONF_MAX_MULTIQ_NUM			3

#define PROC_PATH			            "/proc"

/* sysconfig ���Ͽ� ��ϵǴ� system_type���� �����Ѵ�.
// - FIMD���� ���⿡ ���ǵ� �̸��� �ν��Ѵ�.
//
*/
#define SYSCONF_SYSTYPE_OMP				"OMP"
#define SYSCONF_SYSTYPE_MP				"MP"



#define	SYSCONF_GROUPTYPE_OMP			0
#define	SYSCONF_GROUPTYPE_MP			1

#define MODE_ACTIVE						1
#define MODE_STANDBY					2
#define MODE_BACKUP						3
#define MODE_SWITCHED					4
#define MODE_NOT_EQUIP					5
#define SYSCONF_SYSSTATE_ACTIVE         0
#define SYSCONF_SYSSTATE_BACKUP         1
#define SYSCONF_SYSSTATE_SWITCHED       2
#define SYSCONF_SYSSTATE_STR_ACTIVE     "ACTIVE"
#define SYSCONF_SYSSTATE_STR_STANDBY    "STANDBY"
#define SYSCONF_SYSSTATE_STR_BACKUP     "BACKUP"
#define SYSCONF_SYSSTATE_STR_SWITCHED   "SWITCHED"  /* STANDBY --> ACTIVE */

#ifndef HPUX
#define         SHM_FAILED  -1
#define         MAXSIG      64
#endif

#endif /*__SYSCONF_H__*/

