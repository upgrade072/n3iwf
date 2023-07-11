/*****************************************************************************/
/* THIS SAMPLE PROGRAM IS PROVIDED AS IS. THE SAMPLE PROGRAM AND ANY RESULTS */
/* OBTAINED FROM IT ARE PROVIDED WITHOUT ANY WARRANTIES OR REPRESENTATIONS,  */
/* EXPRESS, IMPLIED OR STATUTORY.                                            */
/*****************************************************************************/

#include "NGAP-PDU-Descriptions-createvalue.h"
#include "NGAP-PDU-Descriptions-printvalue.h"

int main(void)
{
    OssGlobal		w, *world = &w;
    OssBuf		encodedData;
    int			retcode;
    int			pduNum;
    unsigned int	errorCount = 0;
    /* Variables for the generated values */
    NGAP_PDU *	samplevalue_NGAP_PDU;	/* value of 'NGAP_PDU' type */

    /* Initialize the OSS runtime */
    retcode = ossinit(world, NGAP_PDU_Descriptions);
    if (retcode != 0) {
	ossPrint(NULL, "Initialization failed, error %d\n", retcode);
	return retcode;
    }

    /* Set flags */
    ossSetFlags(world, ossGetFlags(world) | OSS_TRAPPING);

    /* Enable skipping fields that are equal to their default values. */
    ossSetFlags(world, ossGetFlags(world) | STRICT_PER_ENCODING_OF_DEFAULT_VALUES);

    /* Turn on the automatic encoding and decoding of open types */
    ossSetFlags(world, ossGetFlags(world) | AUTOMATIC_ENCDEC);

    /* Set the encoding rules */
    ossSetEncodingRules(world, OSS_PER_ALIGNED); /* or OSS_XER or OSS_CXER or OSS_EXER or OSS_JSON */

    /* To avoid printing the resulted encoded value, comment out the following line. */
    ossSetDebugFlags(world, ossGetDebugFlags(world) | PRINT_ENCODER_OUTPUT);

    ossPrint(world, "\n=====================================================================\n");
    ossPrint(world, "Demonstration of procedures with the value 'samplevalue_NGAP_PDU'\n");
    ossPrint(world, "=====================================================================\n");
    /* Create the 'samplevalue_NGAP_PDU' value */
    samplevalue_NGAP_PDU = create_samplevalue_NGAP_PDU(world);
    pduNum = NGAP_PDU_PDU;

    /* Print the value */
    ossPrint(world, "\nThe encoder input value:\n\n");
    print_NGAP_PDU(world, samplevalue_NGAP_PDU);
    ossPrint(world, "\n");

    /* Initialize the output buffer for encoding to make encoder allocate memory */
    encodedData.value = NULL;
    encodedData.length = 0;

    /* Encode the value */
    retcode = ossEncode(world, pduNum, samplevalue_NGAP_PDU, &encodedData);
    /* Free memory had been allocated for the value */
    ossFreePDU(world, pduNum, samplevalue_NGAP_PDU);

    if (retcode != PDU_ENCODED) {
	ossPrint(world, "\nEncode error: %s\n", ossGetErrMsg(world));
    } else {
	/* Set value to NULL to make decoder allocate memory */
	samplevalue_NGAP_PDU = NULL;

	/* Decode the PDU */
	retcode = ossDecode(world, &pduNum, &encodedData, (void **)&samplevalue_NGAP_PDU);
	/* Free memory had been allocated for the encoding */
	ossFreeBuf(world, encodedData.value);

	if (retcode != PDU_DECODED) {
	    ossPrint(world, "\nDecode error: %s\n", ossGetErrMsg(world));
	} else {
	    /* Traverse the decoded value and print its contents */
	    ossPrint(world, "\nThe decoder output:\n\n");
	    print_NGAP_PDU(world, samplevalue_NGAP_PDU);
	    /* Free memory allocated for the decoded value */
	    ossFreePDU(world, pduNum, samplevalue_NGAP_PDU);
	}
    }

    if (retcode != 0)
	errorCount++;

    /* Terminate the OSS runtime */
    ossterm(world);

    return errorCount ? EXIT_FAILURE : EXIT_SUCCESS;
}
