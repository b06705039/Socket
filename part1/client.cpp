#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#define BUFF_SIZE 1024
using namespace std;

void tolowerCase(string& upperCase){
  int i=0;
  char c;
  string s;
  while (upperCase[i]){
    c=upperCase[i];
    s.clear();
    c=tolower(c);
    s.push_back(c);
    upperCase.replace(i,1,s); 
    i++;}

}
int myStrNCmp(const string& s1, const string& s2, unsigned n)
{
   assert(n > 0);
   unsigned n2 = s2.size();
   if (n2 == 0) return -1;
   unsigned n1 = s1.size();
   assert(n1 >= n);
   for (unsigned i = 0; i < n1; ++i) {
      if (i == n2)
         return (i < n)? 1 : 0;
      char ch1 = (isupper(s1[i]))? tolower(s1[i]) : s1[i];
      char ch2 = (isupper(s2[i]))? tolower(s2[i]) : s2[i];
      if (ch1 != ch2)
         return (ch1 - ch2);
   }
   return (n1 - n2);
}
size_t myStrGetTok(const string& str, string& tok, size_t pos = 0,
            const char del = ' ')
{
   size_t begin = str.find_first_not_of(del, pos);
   if (begin == string::npos) { tok = ""; return begin; }
   size_t end = str.find_first_of(del, begin);
   tok = str.substr(begin, end - begin);
   return end;
}
size_t myStrGetTok(const string& str, char* msg, size_t pos = 0,
            const char del = ' ')
{
   size_t begin = str.find_first_not_of(del, pos);
   if (begin == string::npos) { bzero(msg,sizeof(msg));return begin; }
   size_t end = str.find_first_of(del, begin);
   //tok = str.substr(begin, end - begin);
   strcpy(msg, str.substr(begin, end - begin).c_str());
   return end;
}

bool myStr2Int(const string& str, int& num)
{
   num = 0;
   size_t i = 0;
   int sign = 1;
   if (str[0] == '-') { sign = -1; i = 1; }
   bool valid = false;
   for (; i < str.size(); ++i) {
      if (isdigit(str[i])) {
         num *= 10;
         num += int(str[i] - '0');
         valid = true;
      }
      else return false;
   }
   num *= sign;
   return valid;
}


int main(int argc , char *argv[])
{

    int localSocket, recved;
    localSocket = socket(AF_INET , SOCK_STREAM , 0);

    if (localSocket == -1){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));

    char input[100];
    size_t sizet;
    bzero(&input,sizeof(input));
    printf("input:<IP> <PortNum>\n");
    fgets(input, sizeof(input) / sizeof(input[0]), stdin);
    string tempS, inputS(input);
    sizet = myStrGetTok(inputS, tempS, 0);

    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(tempS.c_str());

    int portNum, tempInt;
    sizet = myStrGetTok(inputS, tempS, sizet);
    myStr2Int(tempS, portNum);

    info.sin_port = htons(portNum);


    int err = connect(localSocket,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){ printf("Connection error\n"); return 0; }

    char receiveConnect[BUFF_SIZE] = {};
    bzero(receiveConnect,sizeof(char)*BUFF_SIZE);
    recv(localSocket,receiveConnect,sizeof(char)*BUFF_SIZE,0);
    printf("%s", receiveConnect);


    char message[100];
    char receiveMessage[100] = {};
    char tempC[100];
    char NL[2] = "\n";
    while(fgets(message,  sizeof(message) / sizeof(message[0]), stdin))
    {
      string thismsg(message);
      sizet = myStrGetTok(thismsg, tempS, 0);

      if(myStrNCmp("REGISTER", tempS, 1)==0){
        bzero(message,sizeof(message));
        strcat(message,"REGISTER#");
        sizet = myStrGetTok(thismsg, tempC, sizet);
        strcat(message,tempC);
        strcat(message,NL);
      }
      else if(myStrNCmp("Login", tempS, 1)==0){
        bzero(message,sizeof(message));
        sizet = myStrGetTok(thismsg, tempC, sizet);
        strcat(message,tempC);
        strcat(message,"#");
        sizet = myStrGetTok(thismsg, tempC, sizet);
        strcat(message,tempC);
        strcat(message,NL);
      }
      // else if(myStrNCmp("List", tempS, 1)==0){
      //   bzero(message,sizeof(message));
      //   strcat(message,"List\n");

      // }

      send(localSocket,message,sizeof(message),0);
      
      bzero(receiveMessage,sizeof(char)*BUFF_SIZE);
      recv(localSocket,receiveMessage,sizeof(char)*BUFF_SIZE,0);
      printf("%s", receiveMessage);
      if(strcmp(receiveMessage, "Bye")==0){break;}
      bzero(message,sizeof(message));

    }
    close(localSocket);
    return 0;
}



