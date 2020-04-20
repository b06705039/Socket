#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <map>
#include <pthread.h>
#include <queue>

#define BUFF_SIZE 1024
#define MaxClientNum 10
#define ThreadPoolNum 2

using namespace std;


//-------------------  struct------------------------------------------------//

struct clientData{
  clientData(){}
  clientData(string name, int bal):_name(name),_balance(bal){}
  void print(){cout <<" =>this vliend==name: "<< _name <<" , balance: " << _balance << endl;}

  string _name;
  int _balance;
};

struct connect{
  connect(){}
  connect(char* ip):_ip(ip){}
  void print(){cout <<" =>this connection==port: "<< _port <<" , ip: " << _ip <<" , userName: " << _userName <<  endl;}

  int _port;
  char *_ip;
  string _userName;
};

//------------------- (end) struct-----------------------------------------//


//---------------------- global variable ---------------------------------//
map<string,  clientData> _allClient;
map<int, struct connect> _socCli; // we have name in connect
queue<int> _waitSoc;
char *RegCmd = "REGISTER", *ECmd = "Exit\n", *LCmd = "List\n";

//---------------------- (end) global variable -------------------------//

//---------------------- global function --------------------------------//

size_t myStrGetTok(const string& , string&, size_t , const char );
void * handle_connection(void*);
void writeList(char*);


void print_allClient(){                                                        // delete later
  for(map<string, clientData>::iterator li = _allClient.begin(); li != _allClient.end(); li++){
    cout << li->first << " == " ;
    li->second.print();
  }
}
void print_asocCli(){                                                        // delete later
  for(map<int, struct connect>::iterator li = _socCli.begin(); li != _socCli.end(); li++){
    cout << li->first << " == " ;
    li->second.print();
  }
}

//---------------------- (end) global function ------------------------//

//-------------------  thread implementation------------------------------------------------//

pthread_t thread_pool[ThreadPoolNum];

void * thread_function(void * arg){
	int *pclient;
	while(1){
    sleep(1);
    pclient = NULL;
		if(!_waitSoc.empty()){
			int socNum = _waitSoc.front();
			pclient = &socNum;
			_waitSoc.pop();
		}
		if (pclient!=NULL){
			handle_connection(pclient);
		}
	}
}

//-------------------  (end) thread implementation------------------------------------------------//

//-------------------   main  --------------------------------------------//

int main(int argc, char* argv[]){

    int localSocket, remoteSocket;                               
    struct  sockaddr_in localAddr,remoteAddr;
    int addrLen = sizeof(struct sockaddr_in);  

    for(int i = 0;i<ThreadPoolNum;i++){																			// create thread pool
    	pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    
    if (localSocket == -1){
        printf("socket() call failed!!");
        return 0;
    }

    if(argc!=2){ cerr << "error input" << endl; return 0;}

    localAddr.sin_family = PF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(atoi(argv[1]));
        
     if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         printf("Can't bind() socket");
         return 0;
     }
        
    listen(localSocket , MaxClientNum);
    cout <<  "Waiting for connections\n" ;
    ////////////////////       finish set up         /////////////////////////

    char Message[BUFF_SIZE] = {};

    while(1){    

        remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
        if(remoteSocket ==-1){ 
          cerr << "error connection" << endl;
        }
          int sent;
          strcpy(Message,"Connection accepted\n"); 
          printf("%s\n", Message );
          sent = send(remoteSocket,Message,strlen(Message),0);

          char *clientip = new char[20];
          strcpy(clientip, inet_ntoa(remoteAddr.sin_addr));
          struct connect newCon(clientip);
          _socCli[remoteSocket] = newCon;

          //handle_connection(pclient);												// direactly handle connection -- ok
          

          // pthread_t t;																	// single thread -- ok
          // int* pclient = (int*)malloc(sizeof(int));
          // *pclient = remoteSocket;
          // pthread_create(&t, NULL, handle_connection, pclient);


          // int * pclient = (int*)malloc(sizeof(int));						 // multiple thread -- not ready
          // * pclient = remoteSocket;

          int rm = remoteSocket;
          _waitSoc.push(rm);

   
    }																					// end while
     return 0;
}

//-----------------------------------  (end) main  ---------------------------------------------//


//-----------------------------------  handle connection  -----------------------------------//


void* handle_connection(void* pclient){
  int remoteSocket = *((int*)pclient);
  //free(pclient);  																		// deleted for multiple thread

  char receiveCmd[BUFF_SIZE] = {};                                        // receive command
  char Message[BUFF_SIZE] = {};                                             // for sending

  while(( read(remoteSocket, receiveCmd, sizeof(receiveCmd))) > 0){
    printf("%s", receiveCmd);

    char* pos;
    char* pound = "#";
    pos = strtok(receiveCmd,pound);

     if (strcmp(receiveCmd,ECmd)==0){                                                        // List & Exit
          strcpy(Message, "Bye");
          send(remoteSocket,Message,sizeof(Message),0);
          return NULL;
     }
     else if (strcmp(receiveCmd,LCmd)==0){
        writeList(Message);
     }
    else if(pos != NULL){
      if(strcmp(pos,RegCmd)==0){                                           // register cmd
        pos = strtok(NULL,"\n");
        string name(pos);
        if (_allClient.find(name)==_allClient.end()){                     // see if the name exist already
          clientData newCli(name, 10000);                                 //  add if not exist in _allClient
          _allClient[name] = newCli;
          strcpy(Message,"100 OK\n"); 
        }
        else{  strcpy(Message,"210 FAIL\n");  }
      }                                                                                   // end register
      else{                                                                             // not register, see if it is login
        string name(pos);
        if(_allClient.find(name)!=_allClient.end() ){                        // if find name
          if(!_socCli[remoteSocket]._userName.empty()){             // only one person can login in a time
            strcpy(Message,"220 AUTH_FAIL\n"); }
          else{
             _socCli[remoteSocket]._userName=name;
            pos = strtok(NULL,"\n");
            int portN = atoi(pos);
             _socCli[remoteSocket]._port=portN;
             sprintf(Message, "%d\n", _allClient[name]._balance);       // send balance

          }
        }
        else{  strcpy(Message,"220 AUTH_FAIL\n");  }
      }
    }
    


    cout << "the ready to send msg: " << Message ;      // trytry

    send(remoteSocket,Message,sizeof(Message),0);

    bzero(Message,sizeof(Message));
  }																							// end accept while

return NULL;
}





//----------------------------------- (end) handle connection  -----------------------------------//

//----------------------------------- global function implement  ---------------------------------//

size_t myStrGetTok(const string& str, string& tok, size_t pos = 0,
            const char del = ' ')
{
   size_t begin = str.find_first_not_of(del, pos);
   if (begin == string::npos) { tok = ""; return begin; }
   size_t end = str.find_first_of(del, begin);
   tok = str.substr(begin, end - begin);
   return end;
}

void writeList(char* msg){
  int onlineC=0;
  char clientMsg[BUFF_SIZE] = {};
  char temp[BUFF_SIZE] = {};
  for(map<int, struct connect>::iterator li = _socCli.begin(); li != _socCli.end(); li++){
    if(!li->second._userName.empty()){
      onlineC++;
      strcat(clientMsg,li->second._userName.c_str());             strcat(clientMsg,"#");
      strcat(clientMsg, li->second._ip);                                  strcat(clientMsg,"#");
      sprintf(temp, "%d\n", li->second._port);
      strcat(clientMsg, temp);
    }
  }
  sprintf(msg, "number of accounts online: %d\n", onlineC);
  strcat(msg, clientMsg);
}


//----------------------------------- (end) global function implement  -------------------------//
