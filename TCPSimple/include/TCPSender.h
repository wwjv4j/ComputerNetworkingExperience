#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
#include <deque>
#include <map>

class TCPSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// 下一个发送序号 
	bool waitingState;				// 是否处于等待Ack的状态
	Packet packetWaitingAck;		// 最晚发送的数据包
	int base;						// 窗口当前基准
	int windowLen, nowWindowLen;    // 最大和当前窗口长度
	map<int, Packet> packetCache;   // 缓存窗口中的Packet
	map<int, int> ACKTimes;			// 统计ACK次数
	
public:

	bool getWaitingState();
	bool send(const Message &message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet &ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用

public:
	TCPSender();
	virtual ~TCPSender();
};

#endif

