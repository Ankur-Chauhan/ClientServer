/*
 * File:   main.cpp
 * Author: AnkurChauhan
 *
 * Created on April 19, 2017, 4:12 PM
 */

#include <iostream>
#include "tcpServer.h"
#include "../commonobjects/ConfigReader.h"


TCPServer* g_pcTCPServer;

short RecvMessageCallback(int64_t nKey, char* pcMessage, int nLength)
{
  g_pcTCPServer->ProcessBusinessMsg(nKey, pcMessage, nLength);
  return 1;
}

short ConnectionNotifierCallBack(int64_t nKey)
{
  std::cout<<"Connection Established"<<"\n";
  return 1;
}

short DisConnectionNotifierCallBack(int64_t nKey, int nReason)
{
  std::cout<<"Disconnection"<<"\n";
  return 1;
}

short ProcessSomethingCallBack()
{
  return 1;
}

int main(int argc, char** argv)
{ 
  ConfigReader lcConfigReader(argv[1]);
  const std::string lzServerIPAddr = lcConfigReader.getProperty("SERVER_IP_ADDRESS");
  const int32_t lnServerPort = atoi(lcConfigReader.getProperty("SERVER_PORT").c_str());
  const int32_t lnCoreId = atoi(lcConfigReader.getProperty("CORE_ID").c_str());
  
  TCPServer* lpcTCPServer = new TCPServer();
  g_pcTCPServer = lpcTCPServer ;
  lpcTCPServer->CreateServerListener(lzServerIPAddr.c_str(), lnServerPort, lnCoreId);
  lpcTCPServer->SetConnectionCallback(ConnectionNotifierCallBack);
  lpcTCPServer->SetDisConnectionCallback(DisConnectionNotifierCallBack);  
  lpcTCPServer->SetReceiveCallback(RecvMessageCallback);
  lpcTCPServer->StartTCPServer();
  
  while(true)
  {
    sched_yield();
  }

     
     return 0;
}

