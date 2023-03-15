/*****************************************************************************/
/* File Name   : sample.c                                                    */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : To Demonstrate the use of the Parking Meter Library 				 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include "rwl.h"
#include "rwl_common.h"
#include "csrw.h"

#define	VERSION "1.1"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void testmenu();

//For time performance measurement
struct timeval  g_starttime1, g_starttime2, g_starttime3, g_starttime4, g_starttime5;
struct timeval  g_endtime1, g_endtime2, g_endtime3, g_endtime4, g_endtime5;
struct tm  g_starttimeinfo1, g_starttimeinfo2, g_starttimeinfo3, g_starttimeinfo4, g_starttimeinfo5;
struct tm  g_endtimeinfo1, g_endtimeinfo2, g_endtimeinfo3, g_endtimeinfo4, g_endtimeinfo5;
ULONG g_timeelapse1, g_timeelapse2, g_timeelapse3, g_timeelapse4, g_timeelapse5;
CHAR timer1str[50] = {0};
CHAR timer2str[50] = {0};
CHAR timer3str[50] = {0};
CHAR timer4str[50] = {0};
CHAR timer5str[50] = {0};

void vResetTimer()
{
	g_timeelapse1 = g_timeelapse2 = g_timeelapse3 = g_timeelapse4 = g_timeelapse5 = 0;
	memset(&timer1str[0], 0, sizeof(timer1str));
	memset(&timer2str[0], 0, sizeof(timer2str));
	memset(&timer3str[0], 0, sizeof(timer3str));
	memset(&timer4str[0], 0, sizeof(timer4str));
	memset(&timer5str[0], 0, sizeof(timer5str));
}

void GetStartTime1(CHAR *s)
{
	time_t rawtime;

	memcpy(&timer1str[0], s, strlen(s));
	time(&rawtime);
	g_starttimeinfo1 = *localtime(&rawtime);
	gettimeofday(&g_starttime1, NULL);
}

void GetStartTime2(CHAR *s)
{
	time_t rawtime;

	memcpy(&timer2str[0], s, strlen(s));
	time(&rawtime);
	g_starttimeinfo2 = *localtime(&rawtime);
	gettimeofday(&g_starttime2, NULL);
}

void GetStartTime3(CHAR *s)
{
	time_t rawtime;

	memcpy(&timer3str[0], s, strlen(s));
	time(&rawtime);
	g_starttimeinfo3 = *localtime(&rawtime);
	gettimeofday(&g_starttime3, NULL);
}

void GetStartTime4(CHAR *s)
{
	time_t rawtime;

	memcpy(&timer4str[0], s, strlen(s));
	time(&rawtime);
	g_starttimeinfo4 = *localtime(&rawtime);
	gettimeofday(&g_starttime4, NULL);
}

void GetStartTime5(CHAR *s)
{
	time_t rawtime;

	memcpy(&timer5str[0], s, strlen(s));
	time(&rawtime);
	g_starttimeinfo5 = *localtime(&rawtime);
	gettimeofday(&g_starttime5, NULL);
}

void GetEndTime1(void)
{
	time_t rawtime;

	time(&rawtime);
	g_endtimeinfo1 = *localtime(&rawtime);
	gettimeofday(&g_endtime1, NULL);
	g_timeelapse1 = ((g_endtime1.tv_sec-g_starttime1.tv_sec)*1000000+(g_endtime1.tv_usec-g_starttime1.tv_usec));
}

void GetEndTime2(void)
{
	time_t rawtime;

	time(&rawtime);
	g_endtimeinfo2 = *localtime(&rawtime);
	gettimeofday(&g_endtime2, NULL);
	g_timeelapse2 = ((g_endtime2.tv_sec-g_starttime2.tv_sec)*1000000+(g_endtime2.tv_usec-g_starttime2.tv_usec));
}

void GetEndTime3(void)
{
	time_t rawtime;

	time(&rawtime);
	g_endtimeinfo3 = *localtime(&rawtime);
	gettimeofday(&g_endtime3, NULL);
	g_timeelapse3 = ((g_endtime3.tv_sec-g_starttime3.tv_sec)*1000000+(g_endtime3.tv_usec-g_starttime3.tv_usec));
}

void GetEndTime4(void)
{
	time_t rawtime;

	time(&rawtime);
	g_endtimeinfo4 = *localtime(&rawtime);
	gettimeofday(&g_endtime4, NULL);
	g_timeelapse4 = ((g_endtime4.tv_sec-g_starttime4.tv_sec)*1000000+(g_endtime4.tv_usec-g_starttime4.tv_usec));
}

void GetEndTime5(void)
{
	time_t rawtime;

	time(&rawtime);
	g_endtimeinfo5 = *localtime(&rawtime);
	gettimeofday(&g_endtime5, NULL);
	g_timeelapse5 = ((g_endtime5.tv_sec-g_starttime5.tv_sec)*1000000+(g_endtime5.tv_usec-g_starttime5.tv_usec));
}

void vPrintTimer()
{
	if(g_timeelapse1)
		printf("Time usage for %s = %lds%03ldms%03ldus\n", &timer1str[0], g_timeelapse1/1000000, (g_timeelapse1%1000000)/1000, g_timeelapse1%1000);

	if(g_timeelapse2)
		printf("Time usage for %s = %lds%03ldms%03ldus\n", &timer2str[0], g_timeelapse2/1000000, (g_timeelapse2%1000000)/1000, g_timeelapse2%1000);

	if(g_timeelapse3)
		printf("Time usage for %s = %lds%03ldms%03ldus\n", &timer3str[0], g_timeelapse3/1000000, (g_timeelapse3%1000000)/1000, g_timeelapse3%1000);

	if(g_timeelapse4)
		printf("Time usage for %s = %lds%03ldms%03ldus\n", &timer4str[0], g_timeelapse4/1000000, (g_timeelapse4%1000000)/1000, g_timeelapse4%1000);

	if(g_timeelapse5)
		printf("Time usage for %s = %lds%03ldms%03ldus\n", &timer5str[0], g_timeelapse5/1000000, (g_timeelapse5%1000000)/1000, g_timeelapse5%1000);

	if(g_AckExpired)
		printf("<<<<<COMMAND ACK EXCEED TIME LIMIT>>>>>\n");

	if(g_RespExpired)
		printf("<<<<<RESPONSE EXCEED TIME LIMIT>>>>>\n");
}

INT main()
{
	CHAR 	XFileName[128];
	CHAR	temp[20];
	CHAR	tmpstr[3];
	CHAR 	ch = 0;
	BYTE 	AI[64];
	INT		s32_ret;
	UINT	i = 0;
	UINT 	in1, in2, in3, in4;
	UINT 	u32_tempstr[30];
	Rsp_PollDeductStl pollDeductInfo;
	stDevVer DeviceV;

	setvbuf(stdout, NULL, _IOLBF, 1000000);
	testmenu();

	do {
		do {
			printf("	Enter your choice: -> ");
			ch = getchar();
			if(ch == 0x0A)
				testmenu();
		} while (ch == 0x0A); //line feed
		getchar();
		vResetTimer();
		memset(AI, 0, sizeof(AI));

		switch(ch) {
			case '1':
			{
				printf("**InitComm**\n");

				in1 = 0;
				in2 = 921600;
				printf("Enter port number (default = 0): ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in1 = atoi(temp);
				printf("Enter baudrate (default = 921600): ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in2 = atoi(temp);

				printf("Calling InitComm(%d, %d)...\n", in1, in2);
				GetStartTime1("InitComm");
				s32_ret = InitComm(in1, in2);
				GetEndTime1();
					if (s32_ret == ERR_NOERR)
						printf("InitComm success!\n");
					else
						printf("InitComm fail!\n");
			}
				break;

			case '2':
			{
				printf("**TimeVer**\n");
				printf("Set GMT time 20YYMMDDHHMMSS(default use system time): ");
				memset(u32_tempstr, 0, sizeof(u32_tempstr));
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
				{
					for (i = 0; i < 6; i++)
					{
						strncpy((CHAR *)&tmpstr, (const CHAR *)&temp[i*2], 2);
						u32_tempstr[5-i] = atoi(tmpstr);
					}
				}
				/* Here, we demonstrate the Collection of the Device Particulars */
				GetStartTime1("TimeVer");
				s32_ret = TimeVer((BYTE *) &DeviceV, &u32_tempstr[0]);
				GetEndTime1();
				if (s32_ret == ERR_NOERR) {
					printf("TimeVer Success\n");
					printf("Device ID  : %u\n", DeviceV.DevID);
					printf("Oper ID    : %u\n", DeviceV.OperID);
					printf("Dev Time   : %u\n", DeviceV.DevTime);					//Reader time is base on 1/1/2000
					printf("Comp ID    : %u\n", DeviceV.CompID);
					printf("Key Ver    : %u\n", DeviceV.KeyVer);
					printf("EOD Ver    : %u\n", DeviceV.EODVer);
					printf("BL Ver	   : %u\n", DeviceV.BLVer);
					printf("FIRM Ver   : %u\n", DeviceV.FIRMVer);
					printf("CCHS Ver   : %u\n", DeviceV.CCHSVer);
					printf("Location ID: %u\n", DeviceV.CSSer);
					printf("IntBL Ver  : %u\n", DeviceV.IntBLVer);
					printf("FuncBL Ver : %u\n", DeviceV.FuncBLVer);
					printf("AlertMsgVer: %u\n", DeviceV.AlertMsgVer);
					printf("RWKey Ver  : %u (0x%04X)\n", DeviceV.RWKeyVer, DeviceV.RWKeyVer);
					printf("OTP Ver    : %u\n", DeviceV.OTPVer);
					printf("Reserved   : \n");
					DbgDump((BYTE *)DeviceV.Reserved, sizeof(DeviceV.Reserved));
				} else
						printf("TimeVer fail!\n");
			}
				break;

			case '3':
			{
				printf("**Authenticate**\n");
				GetStartTime1("Authenticate");
				s32_ret = Authenticate();
				GetEndTime1();
				if	(s32_ret == ERR_NOERR)
					printf("Authenticate success\n");
				else
					printf("Authenticate fail!\n");
			}
				break;

			// PollDeduct function
			case '4':
			{
				printf("**Poll+Deduct**\n");

				in1 = 30;
				in2 = 1;
				in3 = 0;
				in4 = 1;
				printf("Enter timeout (default = 30 (3000ms)): ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in1 = atoi(temp);

				memset(temp, 0, sizeof(temp));
				printf("Enter deduct amount in cents (default = 1 = 10 cents): ");
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in2 = atoi(temp);

				AI[0] = 0x01;
				AI[1] = 0x02;
				AI[2] = 0xA1;
				AI[3] = 0xA2;
				AI[4] = 0xA3;
				AI[5] = 0x00;
				AI[6] = 0x00;

				printf("Enter 5-byte service info in HEX (default = 0x0102A1A2A3): ");
				printf("<2-bytes receipt number> <2-bytes bay number> <1-byte optional info>: ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);

				if (temp[0] != '\n')
				{
					for (i = 0; i < 14; i+=2)
					{
						strncpy(tmpstr, &temp[i], 2);
						AI[i/2] = strtol(tmpstr, NULL, 16);
					}
				}

				memset(temp, 0, sizeof(temp));
				printf("Enter Chinese Alert Msg format (default = 0): \n");
				printf("(0 = UTF-16LE, 2 = Big-5, 3 = UTF-8): ");
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in3 = atoi(temp);

				memset(temp, 0, sizeof(temp));
				printf("Continuous PollDeduct (default = 1): ");
				fgets(temp, sizeof(temp), stdin);
				if (temp[0] != '\n')
					in4 = atoi(temp);

				for (i = 0; i < in4; i++) {
					printf("<<<<<<  Deduct times %d  >>>>>>\n", i + 1);
					memset((CHAR*)&pollDeductInfo, 0, sizeof(pollDeductInfo));
					printf("Calling Deduct(%d, 0x%02X%02X%02X%02X%02X, %d)...\n", in1, AI[0], AI[1], AI[2], AI[3], AI[4], in2);
					GetStartTime1("PollDeduct");
					s32_ret = PollDeduct(in1, in2, AI, in3, &pollDeductInfo);
					GetEndTime1();
					AI[1]++;		//increment receipt no.
					if(AI[1] == 0)
						AI[0]++;

					if (s32_ret == ERR_NOERR) {
						printf("Deduction success \n");

						if (in4 == 1) {
							printf("Poll Status = %d\n",pollDeductInfo.pollStat);
							strncpy(temp, (CHAR *)pollDeductInfo.CardNo, sizeof(pollDeductInfo.CardNo));
							temp[sizeof(temp)]='\0';
							printf("Card ID = %s\n",temp);
							printf("Language = %d\n", pollDeductInfo.language);
							printf("Remaining Value before = %d\n", End4(pollDeductInfo.beforeRV));
							printf("Remaining Value after = %d\n", End4(pollDeductInfo.afterRV));
							printf("Alert Tone = %d\n", pollDeductInfo.alertTone);
							printf("Alert Msg = %d\n", pollDeductInfo.alertMsg);
							printf("Eng Alert Msg =\n");
							DbgDump((BYTE *)pollDeductInfo.engAlertMsg, sizeof(pollDeductInfo.engAlertMsg));
							printf("Chi Alert Msg =\n");
							DbgDump((BYTE *)pollDeductInfo.chiAlertMsg, sizeof(pollDeductInfo.chiAlertMsg));
							printf("Reserved = \n");
							DbgDump((BYTE *)pollDeductInfo.Reserved, sizeof(pollDeductInfo.Reserved));
						}
					} else {
						if (in4 == 1) {
							if ((pollDeductInfo.pollStat) || (pollDeductInfo.f_err == 0x20)
								|| (pollDeductInfo.f_err == 0x03))
								printf("Deduction fail!\n");
							else {
								printf("Poll success but deduct fail!\n");
								strncpy(temp, (CHAR *)pollDeductInfo.CardNo, sizeof(pollDeductInfo.CardNo));
								temp[sizeof(temp)]='\0';
								printf("Card ID = %s\n",temp);
								printf("Language = %d\n", pollDeductInfo.language);
								printf("Remaining Value before = %d\n",End4(pollDeductInfo.beforeRV));
							}
						}
						else
							printf("Deduction fail!\n");
					}
					if (in4 > 1) {
						vPrintTimer();
						AntennaOff();
				}
			}
			}
				break;

			// Antenna off function
			case '5':
			{
				printf("**Antenna Off**\n");
				GetStartTime1("AntennaCtrl");
				s32_ret = AntennaOff();
				GetEndTime1();

				if (s32_ret == ERR_NOERR)
					printf("Antenna Off Success\n");
				else
					printf("Antenna Off fail!\n");
			}
				break;

			case '6':
			{
				printf("**WriteID**\n");
				printf("Enter Location ID (default 1234): ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				in1 = 1234;
				if (temp[0] != '\n')
					in1 = atoi(temp);

				GetStartTime1("WriteID");
				s32_ret = WriteID(in1);
				GetEndTime1();
				if (s32_ret == ERR_NOERR)
					printf("Write ID Success\n");
				else
					printf("Write ID fail!\n");
			}
				break;

			case '7':
			{
				printf("**HouseKeeping** (init transfer + content transfer)\n");
				GetStartTime1("Housekeeping");

				GetStartTime2("Initital Tran");
				s32_ret = HouseKeepingInitTran();
				GetEndTime2();

				if (s32_ret != ERR_NOERR) {
					printf("HouseKeeping Init Fail\n");
					break;
				}

				GetStartTime3("Content Tran");
				s32_ret = HouseKeepingContTran();
				GetEndTime3();

				if (s32_ret == ERR_NOERR)
					printf("HouseKeeping Success\n");
				else
					printf("HouseKeeping fail!\n");
				GetEndTime1();
			}
				break;

			case '8':
			{
				printf("**XFile** (init receive + content receive)\n");
				printf("Enter Segment size (default 4000): ");
				memset(temp, 0, sizeof(temp));
				fgets(temp, sizeof(temp), stdin);
				in1 = MAX_SEGMENT_LEN;
				if (temp[0] != '\n')
					in1 = atoi(temp);

				GetStartTime1("XFile");

				GetStartTime2("XFile Init");
				s32_ret = XFileInitRecv(XFileName, in1);
				GetEndTime2();

				if (s32_ret != ERR_NOERR) {
					printf("XFile Init Fail\n");
					break;
				}

				GetStartTime3("XFile Cont");
				s32_ret = XFileContRecv(XFileName, in1);
				GetEndTime3();
				GetEndTime1();

				if (s32_ret == ERR_NOERR) {
					printf("File Receive success\n");
					printf("\n*****************************************************************\n");
					printf("Exchange File Name = %s\n",XFileName);
					printf("*****************************************************************\n\n");
				} else
						printf("File Receive fail!\n");
			}
				break;

			case 'e':
			case 'E':
			{
				printf("**End Session**\n");
				GetStartTime1("End Session");
				s32_ret = EndSession();
				GetEndTime1();
				if (s32_ret == ERR_NOERR)
					printf("End Session success\n");
				else
					printf("End Session fail!\n");
			}
				break;

			case 'M':
			case 'm':
				testmenu();
				break;

			case '0':
				return 0;
				break;

			default:
				printf("You enter %d %c\n",ch, ch);
				testmenu();
		}
		vPrintTimer();
	} while(1);

	return 1;

}

void testmenu()
{
	printf("\n\nLinux Parking Meter SDK Sample Program Version %s\n", VERSION);
	printf("Please Select Appropriate Function. \n\n");
	printf("1)  InitComm\n");
	printf("2)  Time Version\n");
	printf("3)  Authenticate\n");
	printf("4)  Poll Deduct\n");
	printf("5)  Antenna Off\n");
	printf("6)  Write ID\n");
	printf("7)  HouseKeeping (Initial Transfer + Content Transfer)\n");
	printf("8)  XFile (Initial Receive + Content Receive)\n");
	printf("e)  End Session\n");
	printf("m)  Menu\n");
	printf("0)  Exit\n");
}



