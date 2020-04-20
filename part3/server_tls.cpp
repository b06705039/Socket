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

#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define FAIL    -1

#define BUFF_SIZE 1024
#define MaxClientNum 10
#define ThreadPoolNum 2

using namespace std;
//-------------------  ssl ------------------------------------------------//

SSL_CTX* InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = TLSv1_2_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}
//-------------------  (end) ssl function------------------------------------------------//
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

  SSL* _ssl;
  int _port;
  char *_ip;
  string _userName;
};

//------------------- (end) struct-----------------------------------------//


//---------------------- global variable ---------------------------------//
map<string,  clientData> _allClient;
map<SSL, struct connect> _socCli; // we have name in connect
queue<SSL> _waitSoc;
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
// void print_asocCli(){                                                        // delete later
//   for(map<int, struct connect>::iterator li = _socCli.begin(); li != _socCli.end(); li++){
//     cout << li->first << " == " ;
//     li->second.print();
//   }
// }

//---------------------- (end) global function ------------------------//

//-------------------  thread implementation------------------------------------------------//

pthread_t thread_pool[ThreadPoolNum];

void * thread_function(void * arg){
  SSL *pclient;
  while(1){
    sleep(1);
    pclient = NULL;
    if(!_waitSoc.empty()){
      SSL* qssl = &_waitSoc.front();
      pclient = qssl;
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

  // initial ssl
  SSL_CTX *ctx;
  SSL_library_init();
  ctx = InitServerCTX();        /* initialize SSL */
  LoadCertificates(ctx, "mycert.pem", "mycert.pem"); /* load certs */

  // finish initial ssl


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

      // ssl acceot

      // (end) ssl accept
        SSL *ssl;
        remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
        if(remoteSocket ==-1){ 
          cerr << "error connection" << endl;
        }
          int sent;
          ssl = SSL_new(ctx);              /* get new SSL state with context */
          strcpy(Message,"Connection accepted\n"); 
          printf("%s\n", Message );
          SSL_write(ssl, Message, strlen(Message)); /* send reply */

          char *clientip = new char[20];
          strcpy(clientip, inet_ntoa(remoteAddr.sin_addr));
          struct connect newCon(clientip);
          SSL storessl = *ssl;
          _socCli[storessl] = newCon;

          //handle_connection(pclient);												// direactly handle connection -- ok
          
      
        int rm = remoteSocket;
        SSL_set_fd(ssl, rm);      /* set connection socket to SSL state */
        _waitSoc.push(storessl);

   
    }																					// end while
    close(localSocket);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
     return 0;
}

//-----------------------------------  (end) main  ---------------------------------------------//


//-----------------------------------  handle connection  -----------------------------------//


void* handle_connection(void* ssl){
  SSL* thisssl = (SSL*)ssl;
  int bytes;
  //free(pclient);  																		// deleted for multiple thread
  if ( SSL_accept(thisssl) == FAIL ){     /* do SSL-protocol accept */
      ERR_print_errors_fp(stderr);
      return NULL;
  }


  ShowCerts(thisssl);        /* get any certificates */
  char receiveCmd[BUFF_SIZE] = {};                                        // receive command
  char Message[BUFF_SIZE] = {};                                             // for sending

  while(bytes = SSL_read(thisssl, receiveCmd, sizeof(receiveCmd)) /* get request */){
    receiveCmd[bytes] = '\0';
    printf("%s", receiveCmd);

    char* pos;
    char* pound = "#";
    pos = strtok(receiveCmd,pound);

     if (strcmp(receiveCmd,ECmd)==0){                                                        // List & Exit
          strcpy(Message, "Bye");
          SSL_write(thisssl, Message, strlen(Message)); /* send reply */
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
          if(!_socCli[*thisssl]._userName.empty()){             // only one person can login in a time
            strcpy(Message,"220 AUTH_FAIL\n"); }
          else{
             _socCli[*thisssl]._userName=name;
            pos = strtok(NULL,"\n");
            int portN = atoi(pos);
             _socCli[*thisssl]._port=portN;
             _socCli[*thisssl]._ssl=thisssl;

             sprintf(Message, "%d\n", _allClient[name]._balance);       // send balance

          }
        }
        else{  strcpy(Message,"220 AUTH_FAIL\n");  }
      }
    }
    


    cout << "the ready to send msg: " << Message ;      // trytry
    SSL_write(thisssl, Message, strlen(Message)); /* send reply */
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
  for(map<SSL, struct connect>::iterator li = _socCli.begin(); li != _socCli.end(); li++){
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
