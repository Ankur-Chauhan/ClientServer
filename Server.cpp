#include <stdint.h>
#include "tcpServer.h"


TCPServer::TCPServer()
{
  initSpinLock();
}

TCPServer::~TCPServer()
{

}

const int lnHeaderFixSize = sizeof(int16_t);

short TCPServer::CorePin(int coreID)
{
  short status=0;
  int nThreads = std::thread::hardware_concurrency();
  std::cout<<nThreads;
  cpu_set_t set;
  std::cout<<"\nPinning to Core:"<<coreID<<"\n";
  CPU_ZERO(&set);
  
  if(coreID == -1)
  {
    status=-1;
    std::cout<<"CoreID is -1"<<"\n";
    return status;
  }
  
  if(coreID > nThreads)
  {
    std::cout<<"Invalid CORE ID"<<"\n";
    return status;
  }
  
  CPU_SET(coreID,&set);
  if(sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0)
  {
    std::cout<<"Unable to Set Affinity"<<"\n";
    return -1;
  }
  return 1;
}

void TCPServer::initSpinLock(void)
{
  pthread_spin_init(&sLock, PTHREAD_PROCESS_SHARED);
}


int TCPServer::SendMessage(int fdSocket, char* pcMessage, short nLength)
{
  int numSend = send(fdSocket, pcMessage, nLength, MSG_DONTWAIT); 
  std::cout<<"numSend: "<< numSend <<std::endl;
  std::cout<<"nLenth : "<< nLength << std::endl;
  if(numSend != nLength)
  {
    std::cout<<"Message not sent properly"<<"\n";
  }
  return numSend;
}


bool TCPServer::SetClientConfiguration(int nSockFd)
{

  int flags = fcntl(nSockFd, F_GETFL, 0);
  fcntl(nSockFd, F_SETFL, flags | O_NONBLOCK );  
  
  return true;
}



bool TCPServer::CreateServerListener(const char* pcListenerIpAddr, const unsigned short nListenerPort, const int nCoreID)
{
  int lnRetVal = 0;
	if (0 != m_stListenerInfo.m_nListenerFd)
	{
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): Listener socket already created", ERROR_LOCATION);
    return false;
	}

	m_stListenerInfo.m_nListenerFd = socket(AF_INET, SOCK_STREAM, 0);
	if (SYSTEM_CALL_ERROR == m_stListenerInfo.m_nListenerFd)
	{
		//g_cLogger.LogMessage("%s-%s(%d): Error while creating listener socket(%s)", ERROR_LOCATION, strerror(errno)); 
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): while creating listener socket(%s)", ERROR_LOCATION, strerror(errno));    
		    std::cout << m_cErrorString << std::endl;
    m_stListenerInfo.m_nListenerFd = 0;
		return false;
	}

	//lnRetVal = SetListenerConfiguration(m_stListenerInfo.m_nListenerFd);
	if (SYSTEM_CALL_ERROR == lnRetVal)
	{
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): setting listener socket configuration for fd(%d)", ERROR_LOCATION, m_stListenerInfo.m_nListenerFd);    
		close(m_stListenerInfo.m_nListenerFd);
		m_stListenerInfo.m_nListenerFd = 0;
	}
  
	sockaddr_in	lcListenerAddr;
	lcListenerAddr.sin_family = AF_INET;
	lcListenerAddr.sin_port = htons((short)nListenerPort);
	inet_aton(pcListenerIpAddr, &(lcListenerAddr.sin_addr));
	lnRetVal = bind(m_stListenerInfo.m_nListenerFd, (sockaddr*)&lcListenerAddr, sizeof(sockaddr_in));		  
	if (SYSTEM_CALL_ERROR == lnRetVal)
	{
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): binding address to listener socket(%d),error(%s)", ERROR_LOCATION, m_stListenerInfo.m_nListenerFd, strerror(errno)); 
    std::cout << m_cErrorString << std::endl;
     
		close(m_stListenerInfo.m_nListenerFd);
		m_stListenerInfo.m_nListenerFd = 0;
		return false;
	}

  snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): Port(%d) IPAdd(%s) Listener FD (%d)", ERROR_LOCATION, nListenerPort, pcListenerIpAddr, m_stListenerInfo.m_nListenerFd); 
    std::cout << m_cErrorString << std::endl;
	lnRetVal = listen(m_stListenerInfo.m_nListenerFd, SOMAXCONN);
	if (SYSTEM_CALL_ERROR == lnRetVal)
	{
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): listening for FD(%d) error(%s)", ERROR_LOCATION, m_stListenerInfo.m_nListenerFd, strerror(errno)); 
        std::cout << m_cErrorString << std::endl;
		close(m_stListenerInfo.m_nListenerFd);
		m_stListenerInfo.m_nListenerFd = 0;
		return false;
	}
  
    m_nCoreID = nCoreID;

  return true;
}


void TCPServer::AcceptThreadProc1()
{
  int lnCoreRetVal = CorePin(m_nCoreID);
  
  if(lnCoreRetVal!=1)
  {
    snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): Unable to set CORE ID(%d)", ERROR_LOCATION, m_nCoreID);
    std::cout << m_cErrorString << std::endl;
  }

  
  int lnNumRead=0;
	sockaddr_in lstClientAddr;
	int lnAddrLen = sizeof(sockaddr_in);
	int64_t lnNewFd = 0;
  
	if (0 == m_stListenerInfo.m_nListenerFd)
	{
		return;
	}
 
  
  //fd_set read_flags, tempset; // the flag sets to be used
  struct timeval waitd = {0, 0};          // the max wait time for an event

  FD_ZERO(&read_flags);
  int maxfd = m_stListenerInfo.m_nListenerFd;
  
  FD_SET(m_stListenerInfo.m_nListenerFd, &read_flags);
  //FD_SET(STDIN_FILENO, &read_flags);
  
  int  lnFDReady = 0;
  
  //bool lbStatus1 = false;
  //bool lbStatus2 = false;        
  initializeLenArray();
  tagConnectionStatus* lpcConnectionStatus = NULL;  
	while(true)
	{
    memcpy(&tempset, &read_flags, sizeof(tempset));
    int selectRet = select(maxfd + 1, &tempset, (fd_set*)0, (fd_set*)0, &waitd);
    if (selectRet > 0)
    {
      //std::cout<<"select Ret:"<<selectRet<<"\n";
      lnFDReady = selectRet;       
      //Accept new Connection          
      if(FD_ISSET(m_stListenerInfo.m_nListenerFd, &tempset))
      {
        --lnFDReady;
        //accept
        lnNewFd = accept(m_stListenerInfo.m_nListenerFd, (sockaddr*)&lstClientAddr, (socklen_t*)&lnAddrLen);
        std::cout<<"\n"<<"Accept fd"<< lnNewFd <<"\n";
        if (SYSTEM_CALL_ERROR == lnNewFd)
        {
          snprintf(m_cErrorString, sizeof(m_cErrorString), "%s-%s(%d): Error accepting new connection reason (%s).", ERROR_LOCATION, strerror(errno));
          std::cout<< m_cErrorString;
        }
        else
        {
          SetClientConfiguration(lnNewFd);
          lpcConnectionStatus = new tagConnectionStatus();
          lpcConnectionStatus->nCurrentStatus = 1; //CONNECTED
          lpcConnectionStatus->nFd = lnNewFd;
          
          if(!AddConnectionInStore(lpcConnectionStatus))
          {
            std::cout<<"Unable to add to map"<<"\n";
            continue;
          }
          FD_SET(lnNewFd, &read_flags);
          FD_CLR(m_stListenerInfo.m_nListenerFd, &tempset);
          maxfd = (maxfd < lnNewFd)? lnNewFd:maxfd;
          m_FpConnectionNotifierCallBack(lnNewFd);          
        }
      }

      for(int fdSocket = 1; fdSocket <= maxfd ; ++fdSocket)
      {
        if(FD_ISSET(fdSocket, &tempset))
        {
          tagConnectionStatus* lpstConnectionStatus = GetConnectionInStore(fdSocket);
          if(NULL == lpstConnectionStatus)
          {
            continue;
          }
          --lnFDReady;
          lnNumRead = recv( lpstConnectionStatus->nFd, lpstConnectionStatus->stRecvDataUnit.cBuffer, MAX_RECEIVE_MSG_LENGTH, 0);
          
          //std::cout<<"NUMREAD:"<<lnNumRead<<"\n";
          if(lnNumRead > 0)
          { 
            lpstConnectionStatus->stRecvDataUnit.nLength = lnNumRead;            
            memcpy(lpstConnectionStatus->stRecvDataUnit.cReceivedBuffer + lpstConnectionStatus->stRecvDataUnit.nReceivedBytes, 
                  lpstConnectionStatus->stRecvDataUnit.cBuffer, lpstConnectionStatus->stRecvDataUnit.nLength);
            
            lpstConnectionStatus->stRecvDataUnit.nReceivedBytes += lpstConnectionStatus->stRecvDataUnit.nLength;            

            while(1)
            { 
              
              const int lnPendingBufferCount = (lpstConnectionStatus->stRecvDataUnit.nReceivedBytes - lpstConnectionStatus->stRecvDataUnit.nProcessedLength);
              //std::cout << " lnPendingBufferCount " << lnPendingBufferCount<< std::endl;
              if(lnPendingBufferCount <= lnHeaderFixSize) //
              {
                break;
              }              

              int16_t lnRecvMsgCode = *(int16_t*)(lpstConnectionStatus->stRecvDataUnit.cReceivedBuffer + lpstConnectionStatus->stRecvDataUnit.nProcessedLength);
              int16_t lnRecvLength = lnTempMsgLen[lnRecvMsgCode];
              std::cout<< "RecvMsgCode: " << lnRecvMsgCode << " " << lpstConnectionStatus->stRecvDataUnit.nProcessedLength << " RecvLength " << lnRecvLength << std::endl;;
              
              //std::cout<<"ProcessedLength"<<lpstConnectionStatus->stRecvDataUnit.nProcessedLength<<"\n";
              if(lnRecvLength > MAX_RECEIVE_MSG_LENGTH || lnRecvLength <= 0)
              {
                //std::cout<< m_cRecevString<<"\n";
                
                FD_CLR(lpstConnectionStatus->nFd, &read_flags);   
                if (lpstConnectionStatus->nFd == maxfd)
                {
                  while (FD_ISSET(maxfd, &read_flags) == false)
                    maxfd -= 1;                    
                }
                break;
              }
                  
              if(lnRecvLength > (lpstConnectionStatus->stRecvDataUnit.nReceivedBytes - lpstConnectionStatus->stRecvDataUnit.nProcessedLength)) //
              {
                break;
              }
              else
              {

                //std::cout<<"In while Loop "  << lpstConnectionStatus->stRecvDataUnit.nProcessedLength << " RecvLength " << lnRecvLength << std::endl;;                
                m_FpRecvMessageCallback(lpstConnectionStatus->nFd, lpstConnectionStatus->stRecvDataUnit.cReceivedBuffer + lpstConnectionStatus->stRecvDataUnit.nProcessedLength, lnRecvLength);
                lpstConnectionStatus->stRecvDataUnit.nProcessedLength += (lnRecvLength);
                //lpstConnectionStatus->stRecvDataUnit.nUnProcessedLength -= lpstConnectionStatus->stRecvDataUnit.nProcessedLength;
                
                //std::cout<<"In while Loop "  << lpstConnectionStatus->stRecvDataUnit.nProcessedLength << " " << lpstConnectionStatus->stRecvDataUnit.nUnProcessedLength << std::endl;;                                
                
                if(lpstConnectionStatus->stRecvDataUnit.nProcessedLength == lpstConnectionStatus->stRecvDataUnit.nReceivedBytes)
                {
                  lpstConnectionStatus->stRecvDataUnit.nReceivedBytes = 0;
                  lpstConnectionStatus->stRecvDataUnit.nUnProcessedLength = 0;
                  lpstConnectionStatus->stRecvDataUnit.nProcessedLength = 0;  
                  break;
                }
                
              }
            }           
            
          }
          else if(lnNumRead == -1)
          {
            continue;
          }
          else if(lnNumRead ==  0)
          {
            std::cout<<"\nMap:"<< std::endl;
            for(auto iter = m_mapConnStore.begin(); iter != m_mapConnStore.end(); iter++)
            {
              std::cout<<iter->first<<"\t"<<iter->second<<"\n";
            }
            std::cout<<"Socket Connection Broken"<<"\n";
            FD_CLR(fdSocket,&read_flags);
            if (fdSocket == maxfd)
            {
               while (FD_ISSET(maxfd, &read_flags) == false)
                 maxfd -= 1;                    
            }
            
            RemoveConnectionFromStore(fdSocket);
            int lnReasonCode = 0;

            m_FpDisConnectionNotifierCallBack(lnNewFd, lnReasonCode);                      
              
          }
        } //if(FD_ISSET(new_sd, &read_flags))
      }//for(int fdSocket = 1; fdSocket <= maxfd ; ++fdSocket)        
    }//    if (selectRet > 0)
    else
    {
      continue;
    }
  }
  return;
}

void TCPServer::SetConnectionCallback(FpConnectionNotifierCallBack pConnectionNotifierCallBack)
{

  m_FpConnectionNotifierCallBack = pConnectionNotifierCallBack;
}

void TCPServer::SetDisConnectionCallback(FpDisConnectionNotifierCallBack pDisConnectionNotifierCallBack)
{
  m_FpDisConnectionNotifierCallBack = pDisConnectionNotifierCallBack;
}

void TCPServer::initializeLenArray()
{
  lnTempMsgLen[ENUM_LOGON_MESSAGE] = sizeof(LOGIN_PACKET);
  lnTempMsgLen[ENUM_LOGIN_RESPONSE] = sizeof(TCPCLIENT_HEADER);
  lnTempMsgLen[ENUM_TOKEN_DATA] = sizeof(TOKEN_DATA);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOOL_LEVEL] = sizeof(LIMIT_UPLOAD_TOOL_LEVEL_REQ);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOOL_LEVEL_TO_ALL_CONNECTED] = sizeof(LIMIT_UPLOAD_TOOL_LEVEL_REQ);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL] = sizeof(LIMIT_UPLOAD_TOKEN_LEVEL_REQ);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_TO_ALL_CONNECTED] = sizeof(LIMIT_UPLOAD_TOKEN_LEVEL_REQ);
  lnTempMsgLen[ENUM_KILL_SWITCH_ACTIVATION] = sizeof(KILL_SWITCH);
  lnTempMsgLen[ENUM_KILL_RESPONSE_TO_ALL] = sizeof(KILL_SWITCH);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_FOR_BAN_UNBAN] = sizeof(LIMIT_UPLOAD_TOKEN_LEVEL_REQ);
  lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_FOR_BAN_UNBAN_TO_ALL] = sizeof(LIMIT_UPLOAD_TOKEN_LEVEL_REQ);
  lnTempMsgLen[ENUM_START_STOP_REQ] = sizeof(START_STOP);
  lnTempMsgLen[ENUM_START_STOP_RESP] = sizeof(START_STOP);
}

void TCPServer::SetReceiveCallback(FpRecvMessageCallback pRecvMessageCallback)   
{
  m_FpRecvMessageCallback = pRecvMessageCallback ;
}

bool TCPServer::StartTCPServer()
{
  m_cAcceptThread     = std::thread(&TCPServer::AcceptThreadProc1, this);    
  return true;
}

bool TCPServer::AddConnectionInStore(tagConnectionStatus* pcConnectionStatus)
{
  pthread_spin_lock(&sLock);
  std::pair<mapConnStoreIter, bool> lcConnInsertRet;
  lcConnInsertRet = m_mapConnStore.insert(std::pair<int64_t,tagConnectionStatus*>(pcConnectionStatus->nFd, pcConnectionStatus));
  if(lcConnInsertRet.second == false)
  {
    pthread_spin_unlock(&sLock);
    return false;
  }
  pthread_spin_unlock(&sLock);
  return true;
}

tagConnectionStatus* TCPServer::GetConnectionInStore(int64_t nFd)
{
  pthread_spin_lock(&sLock);
  auto lcItr = m_mapConnStore.find(nFd);
  tagConnectionStatus* lpstConnection;
  if(lcItr != m_mapConnStore.end())
  {
    lpstConnection = lcItr->second;
    pthread_spin_unlock(&sLock);
    return lpstConnection;    
  }
  pthread_spin_unlock(&sLock);
  return NULL;
  
}

bool TCPServer::RemoveConnectionFromStore(int64_t nFd)
{
  pthread_spin_lock(&sLock);
  auto lcItr = m_mapConnStore.find(nFd);
  
  if(lcItr != m_mapConnStore.end())
  {
    delete lcItr->second;
    close(nFd);
    m_mapConnStore.erase(nFd); 
    pthread_spin_unlock(&sLock);
    return true;
  }
  pthread_spin_unlock(&sLock);
  return false;
}

bool TCPServer::ProcessBusinessMsg(int nKey, char* pcMessage, short nLength)
{
  TCPCLIENT_HEADER * lpstClientHeader = (TCPCLIENT_HEADER *)pcMessage;
  switch(lpstClientHeader->MsgCode)
  {
    case ENUM_LOGON_MESSAGE:
    {
      TOKEN_END *tokenEnd= new TOKEN_END;
      TOKEN_DATA * lpstTokenData= new TOKEN_DATA;
      
      std::cout<<"LOGON:nKey|"<<nKey<<"|pcMsg|"<<pcMessage<<"|Len|"<<nLength<<"\n";
      
      lpstClientHeader->MsgCode = ENUM_LOGIN_RESPONSE;
      lpstClientHeader->ErrorCode = 0;
      SendMessage(nKey, (char *)lpstClientHeader, lnTempMsgLen[ENUM_LOGIN_RESPONSE]);
      
      std::cout<<"TOKEN DATA:nKey|"<<nKey<<"|pcMsg|"<<pcMessage<<"|Len|"<<nLength<<"\n";
      lpstTokenData->i16_MsgCode = ENUM_TOKEN_DATA;
      SendMessage(nKey, (char *)lpstTokenData, lnTempMsgLen[ENUM_TOKEN_DATA]);
      lpstTokenData->i16_MsgCode = ENUM_TOKEN_DATA;
      SendMessage(nKey, (char *)lpstTokenData, lnTempMsgLen[ENUM_TOKEN_DATA]);
      
      std::cout<<"TOKEN_END_SIZE"<<sizeof(TOKEN_END)<<"\n";
      tokenEnd->MessageCode = 100;
      SendMessage(nKey,(char *)tokenEnd, sizeof(TOKEN_END));
    }
    break;
    
    case ENUM_LIMIT_UPLOAD_TOOL_LEVEL:
    {
      LIMIT_UPLOAD_TOOL_LEVEL_REQ *lToolReq = (LIMIT_UPLOAD_TOOL_LEVEL_REQ *)pcMessage;
      std::cout<<"\nLIMIT_UPLOAD_TOOL_LEVEL";
      lToolReq->TCPHeader.MsgCode = ENUM_LIMIT_UPLOAD_TOOL_LEVEL_TO_ALL_CONNECTED;
      SendMessage(nKey, (char *)lToolReq, lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOOL_LEVEL_TO_ALL_CONNECTED]);
      
    }
    break;
    
    case ENUM_LIMIT_UPLOAD_TOKEN_LEVEL:
    {
      LIMIT_UPLOAD_TOKEN_LEVEL_REQ *lTokenReq = (LIMIT_UPLOAD_TOKEN_LEVEL_REQ *)pcMessage;
      std::cout<<"\nLIMIT_UPLOAD_TOKEN_LEVEL";
      lTokenReq->TCPHeader.MsgCode = ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_TO_ALL_CONNECTED;
      SendMessage(nKey, (char *)lTokenReq, lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_TO_ALL_CONNECTED]);
    }
    break;
    
    case ENUM_KILL_SWITCH_ACTIVATION:
    {
      KILL_SWITCH *lcptrKillSwitch = (KILL_SWITCH *)pcMessage;
      std::cout<<"\nKILL SWITCH"<<"\n";
      lcptrKillSwitch->TCPHeader.MsgCode = ENUM_KILL_RESPONSE_TO_ALL;
      SendMessage(nKey, (char *)lcptrKillSwitch, lnTempMsgLen[ENUM_KILL_RESPONSE_TO_ALL]);
    }
    break;
    
    case ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_FOR_BAN_UNBAN:
    {
      LIMIT_UPLOAD_TOKEN_LEVEL_REQ *lTokenReq = (LIMIT_UPLOAD_TOKEN_LEVEL_REQ *)pcMessage;
      std::cout<<"\nLIMIT_UPLOAD_TOKEN_LEVEL BAN-UNBAN";
      lTokenReq->TCPHeader.MsgCode = ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_FOR_BAN_UNBAN_TO_ALL;
      SendMessage(nKey, (char *)lTokenReq, lnTempMsgLen[ENUM_LIMIT_UPLOAD_TOKEN_LEVEL_FOR_BAN_UNBAN_TO_ALL]);
    }
    break;
    
    case ENUM_START_STOP_REQ:
    {
      TCPCLIENT_HEADER *lpstStartStopHeader = (TCPCLIENT_HEADER *)pcMessage;
      std::cout<<"\nSTART_STOP"<<"\n";
      lpstStartStopHeader->MsgCode = ENUM_START_STOP_RESP;
      SendMessage(nKey, (char *)lpstStartStopHeader, lnTempMsgLen[ENUM_LOGIN_RESPONSE]);
    }
    break;
    
    default:
    {
      std::cout<<"Default nKey|"<<nKey<<"|pcMsg|"<<pcMessage<<"|Len|"<<nLength<<"\n";    
    }
    break;
  }  
  
  return true;
}
