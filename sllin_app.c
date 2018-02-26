#include "sllin_app.h"

unsigned char  ucFuelLevel;
unsigned char  ucOilPress;
signed   char  scOilTemp;
unsigned short uwSpeedometer;
unsigned char  ucBacklight;
int            iAppRunning;
int            iLinSock;
pthread_t LinRecvTh;
pthread_mutex_t RunMutex;

static struct LinMsgStruct TstLinMsgStruct[LinSlavesNum] = {
		{0xC4, 1, 0,   100  },
		{0x64, 1, 0,   65   },
		{0x67, 1, -40, 150  },
		{0xF1, 2, 0,   12000},
		{0x97, 1, 0 ,  100  }
};

void sigterm(int signo)
{
	printf("Signal Catched = %d\n", signo);
	pthread_mutex_lock(&RunMutex);
	iAppRunning = 0;
	pthread_mutex_unlock(&RunMutex);
}

void initGlobals()
{
	ucFuelLevel   = 0;
	ucOilPress    = 0;
	scOilTemp     = -40;
	uwSpeedometer = 0;
	ucBacklight   = 0;
	iAppRunning   = 1;
	iLinSock = -1;
	pthread_mutex_init(&RunMutex, NULL);
	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);
	signal(SIGINT, sigterm);
}

int iLin_InitBus(char * LinInf) {
	struct sockaddr_can LinAddr;
	struct ifreq ifr;

	if (strlen(LinInf) >= IFNAMSIZ) {
		printf("Name of LIN Net Interface '%s' is too long!\n\n", LinInf);
		return STATUS_KO;
	}

	if ((iLinSock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
			perror("LIN socket");
			return STATUS_KO;
	}

	LinAddr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, LinInf);
	if (ioctl(iLinSock, SIOCGIFINDEX, &ifr) < 0) {
		perror("LIN SIOCGIFINDEX");
		return STATUS_KO;
	}
	LinAddr.can_ifindex = ifr.ifr_ifindex;

	//Disable loopback
	/*int loopback = 0;
	setsockopt(iLinSock, SOL_CAN_RAW, CAN_RAW_LOOPBACK,
		   &loopback, sizeof(loopback));*/


	if (bind(iLinSock, (struct sockaddr *)&LinAddr, sizeof(LinAddr)) < 0) {
		perror("LIN bind");
		return STATUS_KO;
	}
	return STATUS_OK;
}

void iLin_DeinitBus() {
   if(iLinSock > 0){
	   close(iLinSock);
	   iLinSock = -1;
   }
}

static int iLin_iSendMsg(unsigned short uwLinId, unsigned char ucLinDataLen, unsigned char * pucLinData)
{
	int iRet = STATUS_OK;
	int mtu = CAN_MTU;
	int maxdlen = CAN_MAX_DLEN;
	static struct canfd_frame frame;
	frame.can_id = (unsigned long)uwLinId;
	frame.can_id &= CAN_SFF_MASK;
	frame.len = ucLinDataLen;
	memcpy(frame.data, pucLinData, ucLinDataLen);
	//set unused LIN Data payload to 0
	if (frame.len < maxdlen)
		memset(&frame.data[frame.len], 0, maxdlen - frame.len);
	if(iLinSock > 0){
		int nbytes = write(iLinSock, &frame, mtu);
		if(nbytes < mtu) {
			printf("Error Sending LIN Frames\n");
			iRet = STATUS_KO;
		}
	}else {
		perror("Lin Socket Down");
		iRet = STATUS_KO;
	}
	return iRet;
}

static int iLin_iRecvMsg(struct LinMsg * UserLinMsg, unsigned long ulTimeout)
{
	int iRet = STATUS_OK;
	static struct canfd_frame frame;
	if(iLinSock > 0){
		fd_set rdfs;
		FD_ZERO(&rdfs);
		FD_SET(iLinSock, &rdfs);
		struct timeval Lints;
		Lints.tv_sec = (unsigned long)(ulTimeout / 1000);
		Lints.tv_usec = (unsigned long)(ulTimeout-(Lints.tv_sec*1000));

		if ((iRet = select(iLinSock+1, &rdfs, NULL, NULL, &Lints)) <= 0) {
			printf("LIN reception Timeout\n");
			return (iRet = STATUS_KO);
		}

		if (FD_ISSET(iLinSock, &rdfs)) {
			int nbytes = read(iLinSock, &frame, CAN_MTU);
			if(nbytes < CAN_MTU) {
				printf("Error reading LIN Frames\n");
				iRet = STATUS_KO;
			}else {
				UserLinMsg->ulLinId   = frame.can_id & CAN_SFF_MASK;
				UserLinMsg->ucDataLen = frame.len;
				memcpy(UserLinMsg->pucData, frame.data, frame.len);
				if (frame.len < CAN_MAX_DLEN)
					memset(&frame.data[frame.len], 0, CAN_MAX_DLEN - frame.len);
			}
		}
	}else {
		perror("Lin Socket Down");
		iRet = STATUS_KO;
	}
	return iRet;

}

int iLin_eUpdateFuelLevel(unsigned char FuelLevel)
{
	int iRet = STATUS_OK;
    unsigned char pucData[8]={0};
    pucData[0] = FuelLevel;
	if(iLin_iSendMsg(TstLinMsgStruct[LinFuelLvl].uwLinId, TstLinMsgStruct[LinFuelLvl].ucLinDataLen, pucData) == STATUS_KO) {
		printf("Fuel Sent KO\n");
		iRet = STATUS_KO;
	}
	return iRet;
}

int iLin_eUpdateOilPress(unsigned short OilPressure)
{
	int iRet = STATUS_OK;
    unsigned char pucData[8]={0};
    pucData[0] = OilPressure;
	if(iLin_iSendMsg(TstLinMsgStruct[LinOilPres].uwLinId, TstLinMsgStruct[LinOilPres].ucLinDataLen, pucData) == STATUS_KO){
		printf("Oil Press Sent KO\n");
		iRet = STATUS_KO;
	}
	return iRet;
}

int iLin_eUpdateOilTemp(signed short OilTemp)
{
	int iRet = STATUS_OK;
    unsigned char pucData[8]={0};
    pucData[0] = (unsigned char)(OilTemp + OilTempOffset);
	if(iLin_iSendMsg(TstLinMsgStruct[LinOilTemp].uwLinId, TstLinMsgStruct[LinOilTemp].ucLinDataLen, pucData) == STATUS_KO){
		printf("Oil Temp Sent KO\n");
		iRet = STATUS_KO;
	}
	return iRet;
}

int iLin_eUpdateSpeedometer(unsigned short Speedometer)
{
	int iRet = STATUS_OK;
    unsigned char pucData[8]={0};
    pucData[0] = (unsigned char)((Speedometer >> 8) & 0xFF);
    pucData[1] = (unsigned char)(Speedometer & 0xFF);
	if(iLin_iSendMsg(TstLinMsgStruct[LinSpeedo].uwLinId, TstLinMsgStruct[LinSpeedo].ucLinDataLen, pucData) == STATUS_KO){
		printf("Speedo Sent KO\n");
		iRet = STATUS_KO;
	}
	return iRet;
}

int iLin_eUpdateBacklight(unsigned char BackLight)
{
	int iRet = STATUS_OK;
    unsigned char pucData[8]={0};
    pucData[0] = BackLight;
	if(iLin_iSendMsg(TstLinMsgStruct[LinBackLight].uwLinId, TstLinMsgStruct[LinBackLight].ucLinDataLen, pucData) == STATUS_KO){
		printf("BackLight Sent KO\n");
		iRet = STATUS_KO;
	}
	return iRet;
}

void * pvLin_iLinRecv_Tsk(void * unused)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	struct LinMsg LinRcvFrame;
	struct timespec LinRcvTime;
	LinRcvTime.tv_sec = 0;
	LinRcvTime.tv_nsec = 50000000;//50ms
	unsigned long timeout_config = (unsigned long)&unused;
	while(iAppRunning){
		if(iLin_iRecvMsg(&LinRcvFrame, timeout_config) == STATUS_KO) {
			printf("Error receiving LIN Frame\n");
			return NULL;
		}else {
			printf("LIN_ID=%lu \t LIN_DLC=%d \n", LinRcvFrame.ulLinId, LinRcvFrame.ucDataLen);
		}
		nanosleep(&LinRcvTime, NULL);
	}
	return NULL;
}

int main(void)
{
	int iRet = STATUS_UNDEF;
	initGlobals();
	struct timespec tv;
	tv.tv_sec  = 0;
	tv.tv_nsec = 100000000; //100ms update
	if(iLin_InitBus((char*)"sllin0") == STATUS_OK)
	{
		printf("LIN Bus Init OK\n");
	} else {
		printf("Error Init LIN Bus\n");
		return STATUS_KO;
	}
    int LinTimeout = LIN_TIMEOUT;
	pthread_create(&LinRecvTh, NULL, pvLin_iLinRecv_Tsk, (void*)&LinTimeout);

	while(iAppRunning) {
    	if(ucFuelLevel <= TstLinMsgStruct[LinFuelLvl].ulMaxVal){
    		ucFuelLevel++;
    	}else {
    		ucFuelLevel = TstLinMsgStruct[LinFuelLvl].slMinVal;
    	}
    	iLin_eUpdateFuelLevel(ucFuelLevel);
    	if(ucOilPress < TstLinMsgStruct[LinOilPres].ulMaxVal) {
    		ucOilPress++;
    	}else {
    		ucOilPress = TstLinMsgStruct[LinOilPres].slMinVal;
    	}
    	iLin_eUpdateOilPress(ucOilPress);
    	if(scOilTemp < TstLinMsgStruct[LinOilTemp].ulMaxVal) {
    		scOilTemp++;
    	}else {
    		scOilTemp = TstLinMsgStruct[LinOilTemp].slMinVal;
    	}
    	iLin_eUpdateOilTemp(scOilTemp);
    	if(uwSpeedometer < TstLinMsgStruct[LinSpeedo].ulMaxVal) {
    		uwSpeedometer++;
    	}else {
    		uwSpeedometer = TstLinMsgStruct[LinSpeedo].slMinVal;
    	}
    	iLin_eUpdateSpeedometer(uwSpeedometer);
    	if(ucBacklight <= TstLinMsgStruct[LinBackLight].ulMaxVal) {
    		ucBacklight++;
    	}else {
    		ucBacklight = TstLinMsgStruct[LinBackLight].slMinVal;
    	}
    	iLin_eUpdateBacklight(ucBacklight);
    	nanosleep(&tv, NULL);
    }
	pthread_cancel(LinRecvTh);
	iLin_DeinitBus();

	return iRet;
}
