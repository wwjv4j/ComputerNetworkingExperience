#include <cmath>
#include <iostream>
#include <deque>
#include "Global.h"
#include "GBNSender.h"

using namespace std;

GBNSender::GBNSender()
{
	this->expectSequenceNumberSend = 0;
	this->waitingState = false;
	this->base = 0;
	this->nowWindowLen = 0;
	this->windowLen = Configuration::MAX_WINDOW_SIZE;
}


GBNSender::~GBNSender()
{
}



bool GBNSender::getWaitingState() {
	return waitingState;
}




bool GBNSender::send(const Message &message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	this->packetCache.push_back(this->packetWaitingAck);

	if(this->expectSequenceNumberSend == this->base) //当该分组是基准分组时，启动发送方定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT,this->packetWaitingAck.seqnum);			
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	
	this->expectSequenceNumberSend ++;
	this->nowWindowLen ++;
	if(this->nowWindowLen == this->windowLen)   //如果到达最大窗口长度，进入等待状态
		this->waitingState = true;		

	cout <<"当前窗口为：[";
	for(auto& it: this->packetCache) {
		cout <<it.seqnum <<", ";
	} 
	cout <<"] "<<endl;
	return true;
}

void GBNSender::receive(const Packet &ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号大于等于发送方base数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum >= this->base) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		
		this->nowWindowLen -= ackPkt.acknum - this->base + 1;
		pns->stopTimer(SENDER, this->base);		//关闭定时器
		this->base = ackPkt.acknum + 1;			// base更新
		this->waitingState = false;

		while(!this->packetCache.empty() && (this->packetCache).front().seqnum < this->base) { //更新缓存 
			this->packetCache.pop_front();
		}

		if(!this->packetCache.empty())
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
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

void GBNSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	cout <<"发送方定时器时间到，重发报文：" <<this->base <<"->" <<this->base + this->packetCache.size() - 1 <<endl;
	pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);			//重新启动发送方定时器
	// 定时器超时，重发窗口内的所有报文
	for(std::deque<Packet>::iterator packet = this->packetCache.begin(); packet != this->packetCache.end(); packet++) { 
		pns->sendToNetworkLayer(RECEIVER, *packet);
	}

}
