#ifndef PROsTO_CON_H_
#define PROTO_CON_H_
#include "proto_comm.h"
#include "proto_con_visitor.h"
#include "proto_stream.h"
#include "proto_packets.h"
#include "send_packet_manager.h"
#include "proto_framer.h"
#include "proto_stream_data_producer.h"
#include <deque>
#include <map>
namespace dqc{
class ProtoCon:public ProtoConVisitor,StreamAckedObserver,
ProtoFrameVisitor,ProtoStreamDataProducer{
public:
    ProtoCon();
    ~ProtoCon();
    virtual void WritevData(uint32_t id,StreamOffset offset,ByteCount len,bool fin) override;
    virtual void OnAckStream(uint32_t id,StreamOffset off,ByteCount len) override;
    ProtoStream *GetOrCreateStream(uint32_t id);
    void Close(uint32_t id);
    void Test();
    PacketNumber AllocSeq(){
        return seq_++;
    }
    //framevisitor
    virtual bool OnStreamFrame(PacketStream &frame) override;
    virtual void OnError(ProtoFramer* framer) override;
    virtual bool OnAckFrameStart(PacketNumber largest_acked,
                                 TimeDelta ack_delay_time) override;
    virtual bool OnAckRange(PacketNumber start, PacketNumber end) override;
    virtual bool OnAckTimestamp(PacketNumber packet_number,
                                ProtoTime timestamp) override;
    virtual bool OnAckFrameEnd(PacketNumber start) override;
    //ProtoStreamDataProducer
    virtual bool WriteStreamData(uint32_t id,
                                 StreamOffset offset,
                                 ByteCount len,
                                 basic::DataWriter *writer) override;
private:
    ProtoStream *CreateStream();
    ProtoStream *GetStream(uint32_t id);
    int Send(ProtoStream *stream,char *buf);
    void Retransmit(uint32_t id,StreamOffset off,ByteCount len,bool fin);
    std::map<uint32_t,ProtoStream*> streams_;
    std::deque<PacketStream> waiting_info_;
    uint32_t stream_id_{0};
    PacketNumber seq_{1};
    SendPacketManager sent_manager_;
    su_addr peer_;
};
}//namespace dqc;
#endif
