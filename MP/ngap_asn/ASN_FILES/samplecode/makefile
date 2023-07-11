###############################################################################
# THIS SAMPLE PROGRAM IS PROVIDED AS IS. THE SAMPLE PROGRAM AND ANY RESULTS   #
# OBTAINED FROM IT ARE PROVIDED WITHOUT ANY WARRANTIES OR REPRESENTATIONS,    #
# EXPRESS, IMPLIED OR STATUTORY.                                              #
###############################################################################
###############################################################################
# This makefile illustrates how to C-compile and link the sample application  #
# created with the -sampleCode option                                         #
###############################################################################

# Set this variable to your location of the OSS ASN.1/C tools
OSSASN1 = /home/n3iwf/ossasn/ossasn1/linux-x86-64.trial/11.1.0.2

ifneq ("$(wildcard $(OSSASN1)/samples/common.mak)","")
include $(OSSASN1)/samples/common.mak
else
$(warning File $$(OSSASN1)/samples/common.mak not found.)
$(error Set OSSASN1 variable to your location of the OSS ASN.1/C tools.)
endif

#
# The OSSPRINT macro provides support for the ossPrintPDU() function
# in TOED-based programs
# The OSSDEBUG macro is defined to obtain a detailed diagnostic message
# in the case of errors in TOED-based programs
#
CPPFLAGS    += -DOSSPRINT -DOSSDEBUG=1 -DOSS_STRICT_PER_ENCODING_OF_DEFAULT_VALUES -I..
# Link with static TOED runtime
OSS_ASN1_LIB = $(OSSTOED)

LIBS        += $(OSS_ASN1_LIB)

all: NGAP-PDU-Descriptions-testmain

NGAP-PDU-Descriptions-createvalue.h NGAP-PDU-Descriptions-printvalue.h: ../NGAP-PDU-Descriptions.h
NGAP-PDU-Descriptions-createvalue.o: NGAP-PDU-Descriptions-createvalue.h NGAP-PDU-Descriptions-createvalue.c
NGAP-PDU-Descriptions-printvalue.o: NGAP-PDU-Descriptions-printvalue.h NGAP-PDU-Descriptions-printvalue.c
NGAP-PDU-Descriptions-testmain.o: NGAP-PDU-Descriptions-createvalue.h NGAP-PDU-Descriptions-testmain.c
../NGAP-PDU-Descriptions.c: ../NGAP-PDU-Descriptions.h

# C-compile the generated codefile
NGAP-PDU-Descriptions.o: ../NGAP-PDU-Descriptions.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

#
# Link the C-compiled output of the ASN.1 compiler with the appropriate
# runtime library.
#
NGAP-PDU-Descriptions-testmain: NGAP-PDU-Descriptions-testmain.o NGAP-PDU-Descriptions-createvalue.o NGAP-PDU-Descriptions-printvalue.o NGAP-PDU-Descriptions.o $(OSS_ASN1_LIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

#
# Clean up.
#
clean:
	$(RM) NGAP-PDU-Descriptions-testmain NGAP-PDU-Descriptions-testmain.o NGAP-PDU-Descriptions-createvalue.o NGAP-PDU-Descriptions-printvalue.o NGAP-PDU-Descriptions.o
