#include <cmath>
#include <iostream>
#include <deque>
#include <map>
#include "Global.h"
#include "TCPSender.h"

using namespace std;

TCPSender::TCPSender()
{
	this->expectSequenceNumberSend = 0;
	this->waitingState = false;
	this->base = 0;
	this->nowWindowLen = 0;
	this->windowLen = 10000;
}


TCPSender::~TCPSender()
{
}



bool TCPSender::getWaitingState() {
	return waitingState;
}




bool TCPSender::send(const Message &message) {
	int dataLen = 0;
	for(int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		if(message.data[i] >= 'A' && message.data[i] <= 'Z') dataLen ++;
	}

	if (this->waitingState || this->nowWindowLen + dataLen > windowLen) { //发送方处于等待确认状态或者缓冲区大小不够
		this->waitingState = true;
		return false;
	}
	
	//设置发送报文内容
	this->packetWaitingAck.acknum = dataLen; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	this->packetCache[this->expectSequenceNumberSend] = this->packetWaitingAck;
	// 发送报文
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

	//启动该报文的定时器
	if(this->expectSequenceNumberSend == this->base) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->base); 		
	}

	// 更新窗口长度和下一个报文序号
	this->expectSequenceNumberSend += dataLen;
	this->nowWindowLen += dataLen;

	cout <<"当前窗口为：[";
	for(auto& it: this->packetCache) {
		cout <<it.second.seqnum <<", ";
	} 
	cout <<"] "<<endl;

	return true;
}

void TCPSender::receive(const Packet &ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号大于等于发送方base数据包序号
	if (checkSum == ackPkt.checksum) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		
		// 更新接收方剩余缓冲区大小
		this->windowLen = ackPkt.seqnum;
		// 更新ACK计数器
		this->ACKTimes[ackPkt.acknum] ++;

		// 判断快速重传和是否更新base
		if(this->ACKTimes[ackPkt.acknum] == 4) {
			pUtils->printPacket("发送方收到4个相同ACK, 重发报文", this->packetCache[ackPkt.acknum]);
			pns->stopTimer(SENDER,ackPkt.acknum);										//首先关闭定时器
			pns->startTimer(SENDER, Configuration::TIME_OUT, ackPkt.acknum);			//重新启动发送方定时器
			pns->sendToNetworkLayer(RECEIVER, this->packetCache[ackPkt.acknum]);	 	//重新发送数据包
		} else if(this->base < ackPkt.acknum) {
			pns->stopTimer(SENDER, this->base);		//关闭定时器
			
			// 根据ack更新缓存和base
			this->base = ackPkt.acknum;
			while(!this->packetCache.empty()) {
				if((this->packetCache.begin())->first < ackPkt.acknum) {
					this->nowWindowLen -= this->packetCache.begin()->second.acknum;
					this->packetCache.erase(this->packetCache.begin()->first);
					this->waitingState = false;
				}
				else break;
			}

			if(this->expectSequenceNumberSend > this->base)
				pns->startTimer(SENDER, Configuration::TIME_OUT, ackPkt.acknum);			//重新启动发送方定时器
		}
			
	} else {
		pUtils->printPacket("发送方没有正确收到发送方的报文,数据校验错误", ackPkt);
	}
}

void TCPSender::timeoutHandler(int seqNum) {
	Packet timeOutPacket = this->packetCache[seqNum];

	// 超时处理
	pUtils->printPacket("发送方定时器时间到，重发超时的报文", timeOutPacket);
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, timeOutPacket);	 						//重新发送数据包

}
