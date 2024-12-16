
#include <vector>
#include "Global.h"
#include "SRReceiver.h"


SRReceiver::SRReceiver():packetCache(Configuration::MAX_WINDOW_SIZE, Packet())
{
	// 本地变量初始化
	this->expectSequenceNumberRcvd = 0;
	this->base = 0;
	this->windowLen = Configuration::MAX_WINDOW_SIZE;
	for(auto& it: this->packetCache) it.seqnum = -1; 	// 缓存初始化

	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRReceiver::~SRReceiver()
{
}

void SRReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号在窗口内
	if (checkSum == packet.checksum) {
		if(packet.seqnum >= this->base) {
			pUtils->printPacket("接收方正确收到发送方的报文", packet);
			this->packetCache[packet.seqnum-base] = packet; 		//缓存报文

			cout <<"当前接收方窗口为：[";
			for(int i = 0; i < this->packetCache.size(); i++) {
				auto it = this->packetCache[i];
				if(it.seqnum != -1)
				cout <<it.seqnum <<", ";
			} 
			cout <<"] "<<endl;
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		
			//若是base报文，将连续的报文向上递交给应用层
			Message msg;
			if(packet.seqnum == this->base) {
				while(this->packetCache[0].seqnum != -1) {
					memcpy(msg.data, this->packetCache[0].payload, sizeof(this->packetCache[0].payload));
					pns->delivertoAppLayer(RECEIVER, msg);
					this->packetCache.erase(this->packetCache.begin());
					this->packetCache.push_back(Packet());
					this->packetCache[this->windowLen-1].seqnum = -1;
					this->base ++;
				}
			}
		}
		else if(packet.seqnum >= this->base - this->windowLen) {
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("重复收到报文，发送最后提交应用层的确认报文", packet);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}
		// this->expectSequenceNumberRcvd ++;
	} else {
		pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
	}
}