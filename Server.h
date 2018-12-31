#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/time.h>
 #include <unistd.h>
#include <sys/fcntl.h>
#include <inttypes.h>
#include <thread>
#include <errno.h>
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
//#include <linux/spinlock>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "EnumTCP.h"


#define ACCEPT_THREAD_ID      1
#define MAX_RECEIVE_MSG_LENGTH_BUFFER 1048576
#define MAX_RECEIVE_MSG_LENGTH      2048
#define TCP_SERVER_ERROR_STR_LEN    512
#define ERROR_LOCATION              __func__, __FILE__, __LINE__
#define SYSTEM_CALL_ERROR           -1



#pragma pack(1)
struct TCPCLIENT_HEADER
{
    int16_t MsgCode;   // 1 - LOGIN_PACKET , 2 - Login Response(Only TCP Header), 3 - TOKEN_DATA on first login (Only message code no header - footer message code 100 ), 4 - LIMIT_UPLOAD_TOOL_LEVEL, 5 - LIMIT_UPLOAD_TOOL_LEVEL to all connected ,
                       // 6 -  LIMT_UPLOAD_TOKEN_LEVEL for exposure , 7 - LIMT_UPLOAD_TOKEN_LEVEL for exposure to all connected users , 8 - TEMP_PORTFOLIO Upload, 9  - F2F_PORTFOLIO as response,
                       // 10 - Kill Switch Activation, 11 - Kill response to all, 12 - LIMT_UPLOAD_TOKEN_LEVEL for Ban - unban ,  13- LIMT_UPLOAD_TOKEN_LEVEL for Ban unban to all connected users
                       // 14 - Start/ Stop Request, 15 - Start/ Stop Response(Only TCP Header),  16 - LIMIT_UPDATE_TOKEN_LEVEL to all connected users
                       // 17 - TRADE_POSITION_PORTFOLIO_UPDATE All Users, 18 - UNSOLICITED_MESSAGE to dealer
    int16_t Login_type;
    int16_t ErrorCode;
    int32_t SeqNo;
};

struct LOGIN_PACKET
{
    TCPCLIENT_HEADER TCPHeader;
    int32_t User_ID;
    int32_t Password;
};

struct LIMIT_UPLOAD_TOOL_LEVEL_REQ
{
    TCPCLIENT_HEADER TCPHeader;
    int32_t Pricerange_Percent;
    int32_t SOV_Lots;
    int32_t MTM_Lots;
    int32_t GrossEposure_Lots;
    int32_t Turnover_Lots;
    int16_t Order_Throttle;
    int16_t AEC;

};

struct LIMIT_UPLOAD_TOKEN_LEVEL_REQ
{
    TCPCLIENT_HEADER TCPHeader;
    int16_t TID;
    int32_t Exposure_Lots;
    int16_t Ban_Unban; // 1 - Ban, 0 - Unban 
};

struct KILL_SWITCH
{
    TCPCLIENT_HEADER TCPHeader;
    int16_t Kill_Activation; // 0 - Deactivated ; 1 Activate 
};

struct START_STOP
{
    TCPCLIENT_HEADER TCPHeader;
    int16_t PID; // 0 - Stop ; 1 Start
    int16_t Start_Stop; 
};

struct LIMIT_UPDATE_TOOL_LEVEL
{
    int32_t MTM_Lots;
    int32_t GrossEposure_Lots;
    int32_t Turnover_Lots;
    int16_t AEC;
};

struct TRADE_POSITION_PORTFOLIO_UPDATE
{

};

struct UNSOLICITED_MESSAGE
{
    int16_t PID;
    int16_t Message_Code;
    int16_t Error_Code;

//case 1 -> PID=0, Message_Code 1 with error code is stop All by RMS 
//case 2 -> PID=<Postfolio id>, Message_Code 2 with error code is stop for that portfolio by RMS
};

struct TOKEN_DATA
{
    int16_t i16_MsgCode;
    int64_t i64_MWPL;
    int64_t i64_Issuedcap;
    int32_t i32_Token;
    int16_t i16_AssetTID;
    int32_t i32_ClosePrice; 
    int32_t i32_TickSize;
    int32_t i32_LotSize;
    int32_t i32_HiDPR;
    int32_t i32_LowDPR;
    uint32_t iu32_ExpiryDate;  
    int32_t i32_StrikePrice;
    int16_t i16_Token_Index_Value;
    int16_t i16_Segment;      // 1 - CM, 2 - FO
    int16_t i16_SegmentGroup; // 1 - CM, 2 - FUTIDX, 3 - FUTSTK, 4 - OPTIDX, 5 - OPTSTK    
    int16_t i16_StreamID; 
    int16_t i16_CALevel;  
    char Symbol[11];
    char Series[3];
    char InstrumentName[7];
    char OptionType[3];
    char Banlist;
    int32_t i32_AssetToken;
    int32_t i32_FreezeQty;                                     // For CM, FO  
    char TradingSymbols[26]; 
};

struct TOKEN_END
{
  int16_t MessageCode;
};

#pragma pack()




typedef struct tagListenerInfo
{
  int                m_nEpollAcceptFd;
  int                m_nListenerFd;
  int                m_nShutdown;		
  std::atomic <int>  m_inTotalConnections;    
		
  tagListenerInfo() : m_nEpollAcceptFd(0), m_nListenerFd(0), m_nShutdown(0)
  {
    m_inTotalConnections = 0;	
  }

  tagListenerInfo(const tagListenerInfo& other) :	m_nEpollAcceptFd(other.m_nEpollAcceptFd),
                                                  m_nListenerFd(other.m_nListenerFd),
                                                  m_nShutdown(other.m_nShutdown)
  {
    int lnTotalConnections = other.m_inTotalConnections;
    m_inTotalConnections = lnTotalConnections;
  }

  tagListenerInfo& operator=(const tagListenerInfo& other)
  {
    if (&other != this)
    {
      m_nEpollAcceptFd        = other.m_nEpollAcceptFd;
      m_nListenerFd           = other.m_nListenerFd;
      m_nShutdown             = other.m_nShutdown;
      int lnTotalConnections  = other.m_inTotalConnections;      
      m_inTotalConnections    = lnTotalConnections;
    }
    return *this;
  }
}tagListenerInfo;




struct tagRecvDataUnit
{
  char          cBuffer[MAX_RECEIVE_MSG_LENGTH];
  char          cReceivedBuffer[MAX_RECEIVE_MSG_LENGTH_BUFFER];
  int           nLength;    
  int           nPendingLength;  
  int           nProcessedLength;
  int           nUnProcessedLength;    
  int           nReceivedBytes;      
  
  
  void Reset()
  {
    nReceivedBytes      = 0;
    nPendingLength      = 0;  
    nLength             = 0;
    nProcessedLength    = 0;
    nUnProcessedLength  = 0;
    memset(cReceivedBuffer, 0,MAX_RECEIVE_MSG_LENGTH );
    memset(cBuffer, 0, MAX_RECEIVE_MSG_LENGTH);  
  }
  
  private:
    void Copy(const tagRecvDataUnit& stRecvDataUnit)
    {
      nLength        = stRecvDataUnit.nLength;
      nPendingLength = stRecvDataUnit.nPendingLength;     
      memcpy(cBuffer, stRecvDataUnit.cBuffer, nPendingLength);    
      return;      
    }
};



struct tagConnectionStatus
{
  int64_t           nFd;
  short             nCurrentStatus; 
  tagRecvDataUnit   stRecvDataUnit;
};

typedef std::unordered_map<int64_t, tagConnectionStatus*> mapConnStore;
typedef mapConnStore::iterator mapConnStoreIter;


typedef short (* FpRecvMessageCallback)(int64_t nKey, char* pcMessage, int nLength);
typedef short (* FpConnectionNotifierCallBack)(int64_t nKey);
typedef short (* FpDisConnectionNotifierCallBack)(int64_t nKey, int nReason);
typedef short (* FpProcessSomethingCallBack)();


class TCPServer
{

public:
  TCPServer();
  ~TCPServer();
  
  public:
    bool  CreateServerListener(const char* pcListenerIpAddr, const unsigned short nListenerPort, const int nCoreID);
    void  SetConnectionCallback(FpConnectionNotifierCallBack pConnectionNotifierCallBack);
    void  SetDisConnectionCallback(FpDisConnectionNotifierCallBack pDisConnectionNotifierCallBack);    
    void  SetReceiveCallback(FpRecvMessageCallback pRecvMessageCallback);    
    bool  StartTCPServer();
    int   SendMessage(int fdSocket, char* pcMessage, short nLength);
    void  initializeLenArray();
    bool  ProcessBusinessMsg(int fdSocket, char* pcMessage, short nLength);               
    
  private:
    void  initSpinLock(void);                                                             //avoiding concurrency related issues
    short CorePin(int coreID);                                                            //core pinning
    bool  RemoveConnectionFromStore(int64_t nFd);                                         //removing an object from map
    tagConnectionStatus* GetConnectionInStore(int64_t nFd);                               //finding a fd from unordered map
    bool  AddConnectionInStore(tagConnectionStatus* pcConnectionStatus);                  //Adding a new in unordered map
    void  AcceptThreadProc1();                                                            
    bool  SetClientConfiguration(int nSockFd);

    
  private:  
    int                           lnTempMsgLen[1+ ENUM_UNSOLICITED_MESSAGE_TO_DEALER] = {0};      //Array that stores size of paths in array
    int                           m_nListenerFd;                                          
    
    int                           m_nRecvBufferSize;
    int                           m_nSendBufferSize;
    int                           m_nNumberOfConnections;
    int                           m_nCoreID;                                               //Pinning to this core
    
    fd_set                        read_flags;                                              //Read flags 
    fd_set                        tempset; 
    
    tagListenerInfo               m_stListenerInfo;                                        //Contains Listener Info
    std::thread                   m_cAcceptThread;
    
    pthread_spinlock_t            sLock;
    std::mutex                    mLock;
    
    mapConnStore                  m_mapConnStore;                                           //stores fd->Connection Status
    mapConnStoreIter              m_mapConnStoreIter;                                       //Iterator of above unordered map

    FpRecvMessageCallback             m_FpRecvMessageCallback;
    FpConnectionNotifierCallBack      m_FpConnectionNotifierCallBack;
    FpDisConnectionNotifierCallBack   m_FpDisConnectionNotifierCallBack;    
    FpProcessSomethingCallBack        m_FpProcessSomethingCallBack;
    
//  char                          m_cCoreErrorString[TCP_SERVER_ERROR_STR_LEN + 1];
    char                          m_cErrorString[TCP_SERVER_ERROR_STR_LEN + 1];
//  char                          m_cRecevString[TCP_SERVER_ERROR_STR_LEN + 1];
//  char                          m_cAcceptString[512 + 1];    
};


#endif
