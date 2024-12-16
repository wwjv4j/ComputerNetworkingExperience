
#include <map>
#include <vector>
#include "Global.h"
#include "TCPReceiver.h"


TCPReceiver::TCPReceiver()
{
	// 本地变量初始化
	this->expectSequenceNumberRcvd = 0;
	this->base = 0;
	this->windowLen = Configuration::MAX_WINDOW_SIZE;

	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//该字段此处代表剩余缓存空间大小
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


TCPReceiver::~TCPReceiver()
{
}

void TCPReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号在窗口内
	if (checkSum == packet.checksum) {
		if(packet.seqnum >= this->base) {
			pUtils->printPacket("接收方正确收到发送方的报文", packet);
			this->packetCache[packet.seqnum] = packet; 		//缓存报文
		
			//若是base报文，将连续的报文向上递交给应用层
			Message msg;
			vector<int> deleteKey; 																//需要删除的缓存
			if(packet.seqnum == this->base) {
				for(auto& pair: this->packetCache) {
					if(pair.second.seqnum == this->base) {
						deleteKey.push_back(this->base);
						memcpy(msg.data, pair.second.payload, sizeof(pair.second.payload));		//向上递交给应用层
						pns->delivertoAppLayer(RECEIVER, msg);
						this->base += pair.second.acknum; 										//更新base
					}
					else break;
				}
				//更新缓存，删除已经提交的packet
				for(auto& key: deleteKey) {
					this->packetCache.erase(key);
				}
			}
		}

		// 根据base设计ACK报文 acknum = base
		map<int, Packet>::reverse_iterator endPacket;
		if(!this->packetCache.empty()) endPacket = this->packetCache.rbegin();
		lastAckPkt.acknum = this->base; //确认序号等于base,即希望收到的序号
		lastAckPkt.seqnum = this->packetCache.empty()? windowLen: windowLen - (endPacket->second.seqnum + endPacket->second.acknum - this->base); //剩余缓存空间大小
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		// this->expectSequenceNumberRcvd ++;
	} else {
		pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
	}

}