#include <cmath>
#include <iostream>
#include <deque>
#include "Global.h"
#include "SRSender.h"

using namespace std;

SRSender::SRSender()
{
	this->expectSequenceNumberSend = 0;
	this->waitingState = false;
	this->base = 0;
	this->nowWindowLen = 0;
	this->windowLen = Configuration::MAX_WINDOW_SIZE;
}


SRSender::~SRSender()
{
}



bool SRSender::getWaitingState() {
	return waitingState;
}




bool SRSender::send(const Message &message) {
	if (this->waitingState) { //发送方处于等待确认状态
		cout <<this->nowWindowLen <<endl;
		return false;
	}
	static int i = 0;
	
	//设置发送报文内容
	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	// 发送报文并启动定时器
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	// cout <<this->base <<endl;
	this->packetCache.push_back(this->packetWaitingAck);
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum); 		//启动该报文的定时器
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	
	// 更新窗口长度和下一个报文序号
	this->expectSequenceNumberSend ++;
	this->nowWindowLen ++;

	if(this->nowWindowLen == this->windowLen)   //如果到达最大窗口长度，进入等待状态
		this->waitingState = true;		
	cout <<"当前发送方窗口为：[";
	for(auto& it: this->packetCache) {
		cout <<it.seqnum <<", ";
	} 
	cout <<"] "<<endl;
	return true;
}

void SRSender::receive(const Packet &ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号大于等于发送方base数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= this->base) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		
		// 从缓存中删除该packet
		for (auto it = this->packetCache.begin(); it != this->packetCache.end(); ++it) {
			if (it->seqnum == ackPkt.acknum) {
				this->packetCache.erase(it);
				break;
			}
		}
	
		pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器

		// 如果当前ack序号是base，更新base为缓存中第一个packet,并更新等待信号
		if(ackPkt.acknum == this->base) {
			if(!this->packetCache.empty()) {
				this->nowWindowLen -= this->packetCache.front().seqnum - this->base;
				this->base = this->packetCache.front().seqnum;			// base更新
			} else {
				this->base += this->nowWindowLen;
				this->nowWindowLen = 0;
			}

			this->waitingState = false;
		}

	} else if(checkSum != ackPkt.checksum) {
		pUtils->printPacket("发送方没有正确收到发送方的报文,数据校验错误", ackPkt);
	}
	// else {
	// 	pUtils->printPacket("发送方没有正确收到确认，重发上次发送的报文", this->packetWaitingAck);
	// 	pns->stopTimer(SENDER, this->packetWaitingAck.seqnum);									//首先关闭定时器
	// 	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//重新启动发送方定时器
	// 	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//重新发送数据包

	// }
}

void SRSender::timeoutHandler(int seqNum) {
	Packet timeOutPacket;

	//从缓存中找到超时报文
	for(auto it = this->packetCache.begin(); it != this->packetCache.end(); ++it) {
		if(it->seqnum == seqNum) {
			timeOutPacket = *it;
		}
	}
	// 超时处理
	pUtils->printPacket("发送方定时器时间到，重发超时的报文", timeOutPacket);
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, timeOutPacket);	 						//重新发送数据包

}
