#include "received_packet_manager.h"
namespace dqc{
ReceivdPacketManager::ReceivdPacketManager()
:time_largest_observed_(ProtoTime::Zero()){}
void ReceivdPacketManager::RecordPacketReceived(PacketNumber seq,ProtoTime receive_time){
    if(time_largest_observed_==ProtoTime::Zero()){
        ack_frame_.largest_acked=seq;
        ack_frame_.packets.Add(seq);
        time_largest_observed_=receive_time;
    }else{
        if(seq>LargestAcked(ack_frame_)){
            ack_frame_.packets.Add(seq);
            ack_frame_.largest_acked=seq;
            time_largest_observed_=receive_time;
        }
    }
}
const AckFrame &ReceivdPacketManager::GetUpdateAckFrame(ProtoTime &now){
    if(time_largest_observed_==ProtoTime::Zero()){
        ack_frame_.ack_delay_time=TimeDelta::Infinite();
    }else{
        ack_frame_.ack_delay_time=now-time_largest_observed_;
    }
    return ack_frame_;
}
void ReceivdPacketManager::DontWaitForPacketsBefore(PacketNumber least_unacked){
    bool update_stop_waiting=false;
    if(0==peer_least_packet_awaiting_ack_.ToUint64()){
        peer_least_packet_awaiting_ack_=least_unacked;
        update_stop_waiting=true;
    }else{
        if(least_unacked>peer_least_packet_awaiting_ack_){
            peer_least_packet_awaiting_ack_=least_unacked;
            update_stop_waiting=true;
        }
    }
    if(update_stop_waiting){
        ack_frame_.packets.RemoveUpTo(least_unacked);
    }
}
}
