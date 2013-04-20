/*************************
[CodeBlue] COMBridge
@begin 27012010
@version 0.2 27012010

Dedicated to Anja.
Happy Birthday!

TODO:
show num of clients active
param for maxusers
**************************/

#pragma comment( lib, "wsock32.lib" ) 
#include <winsock2.h>
#include <stdio.h> 
#include <windows.h>

#define MAX_CLIENTS 180


int startWinsock(){
	WSADATA wsa; 
	return WSAStartup(MAKEWORD(2,0),&wsa); 
}

int help(){
	printf("[CodeBlue] COMBridge Version 0.2\n");
	printf("Valid Arguments:\n");
	printf("\t-p<tcpport>\tTCP Server Port\n");
	printf("\t-b<baudrate>\tCOM Port Baud Rate\n");
	printf("\t-c<portno>\tCOM Port Number\n");
	printf("\t-w<param>\tCOM Port Write Access\n");
	printf("\t\tWhere <param> is one of the following\n");
	printf("\t\t\tn\tnone\n");
	printf("\t\t\ta\tall\n");
	printf("\t\t\t<num>\tConnection ID\n\n");
	printf("Example: combridge -p8080 -b8400 -c14 -wn\n");
	printf("\tConnects to COM14 with 8400 Baud\n\tand streams on Port 8080 with no write access\n");
	printf("\n\nCOMPILED: %s %s",__DATE__,__TIME__);
	return 1;
}

int main(int argc, char* argv[]){
	int serverPort=9064;
	char buf[256]; 
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;
	HANDLE hCom;
	char str_com[10];
	int comPort = 1;
	DWORD eventMask;
	char szBuf;
	DWORD dwIncomingReadSize;
	unsigned long bs_noop;
	int comBaudrate=4800;
	int writeAccess=-2; //-1=all, -2=none
	
	if(argc==1){
		exit(help());
	}
	
	for(int i=1;i<argc;i++){
		if(argv[i][0]=='-'){
			switch(argv[i][1]){
				case 'P':
				case 'p':
					serverPort=strtol(&argv[i][2],NULL,10);
					break;
				case 'b':
				case 'B':
					comBaudrate=strtol(&argv[i][2],NULL,10);
					break;
				case 'c':
				case 'C':
					comPort=strtol(&argv[i][2],NULL,10);
					break;
				case 'w':
				case 'W':
					switch(argv[i][2]){
						case 'N':
						case 'n':
							writeAccess=-2;
							break;
						case 'a':
						case 'A':
							writeAccess=-1;
							break;
						default:
							writeAccess=strtol(&argv[i][2],NULL,10);
							break;
					}
					break;
				case 'h':
				case 'H':
					exit(help());
					break;
			}
		}
	}
	
	
	printf("[CodeBlue] COMBridge Version 0.2\n");
	
	/*TCP SOCKET CODE*/
	printf("\tOpening TCP Socket\n");
	int rc;
	if((rc=startWinsock())!=0){
		printf("\tError starting WinSock\n");
		exit(1);
	}
	
	SOCKET acceptSocket=socket(AF_INET,SOCK_STREAM,0);
	if(acceptSocket==INVALID_SOCKET){
		printf("\tError creating socket\n");
		exit(1);
	} 
  
	SOCKADDR_IN addr; 
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET; 
	addr.sin_port=htons(serverPort); 
	addr.sin_addr.s_addr=INADDR_ANY;
	rc=bind(acceptSocket,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN));
	if(rc==SOCKET_ERROR){
		printf("\tCould not bind socket\n");
		exit(1);
	} 
	rc=listen(acceptSocket,10); 
	if(rc==SOCKET_ERROR){
		printf("\tlisten() failed\n");
		exit(1);
	}
  
	FD_SET fdSet; 
	
	SOCKET clients[MAX_CLIENTS]; 
	for(int i=0;i<MAX_CLIENTS;i++){
		clients[i]=INVALID_SOCKET;
	}
	
	printf("\tTCP Server ready on Port %d\n",serverPort);
	
	
	/*COM PORT CODE*/
	sprintf(str_com, "\\\\.\\COM%d\0", comPort);
	printf("\tConnecting to COM%d\n",comPort);
	hCom = CreateFile(str_com,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (hCom == INVALID_HANDLE_VALUE){
		printf("\tFailed @ CreateFile");
		exit(1);
	}
 
	DCB dcbConfig;
	if(GetCommState(hCom, &dcbConfig)){
		dcbConfig.BaudRate = comBaudrate;
		dcbConfig.ByteSize = 8;
		dcbConfig.Parity = NOPARITY;
		dcbConfig.StopBits = ONESTOPBIT;
		dcbConfig.fBinary = TRUE;
		dcbConfig.fParity = TRUE;
	}
	else{
    	printf("\tFailed @ GetCommState");
		exit(1);
	}

	if(!SetCommState(hCom, &dcbConfig)){
		printf("\tFailed @ SetCommState");
		exit(1);
	}

	COMMTIMEOUTS commTimeout;

	if(GetCommTimeouts(hCom, &commTimeout)){
		commTimeout.ReadIntervalTimeout=3;
		commTimeout.ReadTotalTimeoutConstant=3;
		commTimeout.ReadTotalTimeoutMultiplier=2;
		commTimeout.WriteTotalTimeoutConstant=3;
		commTimeout.WriteTotalTimeoutMultiplier=2;
	}
	else{
		printf("\tFailed @ GetCommTimeouts");
		exit(1);
	}

	if(!SetCommTimeouts(hCom, &commTimeout)){
		printf("\tFailed @ SetCommTimeouts");
		exit(1);
	}
	
	if(!SetCommMask(hCom, EV_RXCHAR)){
		printf("\tFailed @ SetCommMask");
		exit(1);
	}
	

	printf("\tCOM Connection ready on COM%d (%d Baud)\n",comPort,comBaudrate);
	printf("\tWrite Access granted for ");
	switch(writeAccess){
		case -1:
			printf("all");
			break;
		case -2:
			printf("none");
			break;
		default:
			printf("Client ID %d",writeAccess);
			break;
	}
	printf("\n\t--------------------------\n\tNOW STREAMING\n");	
	
	
	/*MAIN LOOP*/
	while(1){
		FD_ZERO(&fdSet); // clear
		FD_SET(acceptSocket,&fdSet); //add
		for(int i=0;i<MAX_CLIENTS;i++){
			if(clients[i]!=INVALID_SOCKET){
				FD_SET(clients[i],&fdSet);
			}
		}
		
		rc=select(0,&fdSet,NULL,NULL,&timeout);
		if(rc==SOCKET_ERROR){
			printf("\tselect() failed\n");
			exit(1);
		}
		
		if(FD_ISSET(acceptSocket,&fdSet)){
			for(int i=0;i<MAX_CLIENTS;i++){
				if(clients[i]==INVALID_SOCKET){
					clients[i]=accept(acceptSocket,NULL,NULL);
					printf("\tClient %d connected\n",i);
					break;
				}
			}
		}
		
		for(int i=0;i<MAX_CLIENTS;i++){
			if(clients[i]==INVALID_SOCKET){
				continue;
			} 
		  
			if(FD_ISSET(clients[i],&fdSet)){
				rc=recv(clients[i],buf,1,0);
				buf[rc]=0;
				if(rc==0 || rc==SOCKET_ERROR){
					closesocket(clients[i]);
					clients[i]=INVALID_SOCKET;
					printf("\tClient %d disconnected\n",i);
				}
				else{
					if(writeAccess==i||writeAccess==-1){
						WriteFile(hCom,buf,1,&bs_noop,NULL);
						if(bs_noop!=1){
							printf("\tError writing to COM (Issued by %d)\n",buf,i);
						}
					}
				}
			}
		}
		if(ReadFile(hCom, &szBuf, 1, &dwIncomingReadSize, NULL)!=0){
			if(dwIncomingReadSize > 0){
				for(int i=0;i<MAX_CLIENTS;i++){
					if(clients[i]==INVALID_SOCKET){
						continue;
					} 
					send(clients[i],&szBuf,1,0);
				}
			}
		}
	}/*main loop*/
	
	
	
	return 0;
}