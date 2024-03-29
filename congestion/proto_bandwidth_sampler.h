#pragma once
#include "proto_types.h"
#include "proto_time.h"
#include "proto_bandwidth.h"
#include "packet_number_indexed_queue.h"
namespace dqc{
// A subset of BandwidthSampler::ConnectionStateOnSentPacket which is returned
// to the caller when the packet is acked or lost.
struct SendTimeState {
  SendTimeState()
      : is_valid(false),
        is_app_limited(false),
        total_bytes_sent(0),
        total_bytes_acked(0),
        total_bytes_lost(0) {}

  SendTimeState(bool is_app_limited,
                QuicByteCount total_bytes_sent,
                QuicByteCount total_bytes_acked,
                QuicByteCount total_bytes_lost)
      : is_valid(true),
        is_app_limited(is_app_limited),
        total_bytes_sent(total_bytes_sent),
        total_bytes_acked(total_bytes_acked),
        total_bytes_lost(total_bytes_lost) {}

  SendTimeState(const SendTimeState& other) = default;

  // Whether other states in this object is valid.
  bool is_valid;

  // Whether the sender is app limited at the time the packet was sent.
  // App limited bandwidth sample might be artificially low because the sender
  // did not have enough data to send in order to saturate the link.
  bool is_app_limited;

  // Total number of sent bytes at the time the packet was sent.
  // Includes the packet itself.
  QuicByteCount total_bytes_sent;

  // Total number of acked bytes at the time the packet was sent.
  QuicByteCount total_bytes_acked;

  // Total number of lost bytes at the time the packet was sent.
  QuicByteCount total_bytes_lost;
};

struct BandwidthSample {
  // The bandwidth at that particular sample. Zero if no valid bandwidth sample
  // is available.
  QuicBandwidth bandwidth;

  // The RTT measurement at this particular sample.  Zero if no RTT sample is
  // available.  Does not correct for delayed ack time.
  TimeDelta rtt;

  // States captured when the packet was sent.
  SendTimeState state_at_send;

  BandwidthSample()
      : bandwidth(QuicBandwidth::Zero()), rtt(TimeDelta::Zero()) {}
};

// An interface common to any class that can provide bandwidth samples from the
// information per individual acknowledged packet.
class BandwidthSamplerInterface {
 public:
  virtual ~BandwidthSamplerInterface() {}

  // Inputs the sent packet information into the sampler. Assumes that all
  // packets are sent in order. The information about the packet will not be
  // released from the sampler until it the packet is either acknowledged or
  // declared lost.
  virtual void OnPacketSent(
      ProtoTime sent_time,
      QuicPacketNumber packet_number,
      QuicByteCount bytes,
      QuicByteCount bytes_in_flight,
      HasRetransmittableData has_retransmittable_data) = 0;

  // Notifies the sampler that the |packet_number| is acknowledged. Returns a
  // bandwidth sample. If no bandwidth sample is available,
  // QuicBandwidth::Zero() is returned.
  virtual BandwidthSample OnPacketAcknowledged(
      ProtoTime ack_time,
      QuicPacketNumber packet_number) = 0;

  // Informs the sampler that a packet is considered lost and it should no
  // longer keep track of it.
  virtual SendTimeState OnPacketLost(QuicPacketNumber packet_number) = 0;

  // Informs the sampler that the connection is currently app-limited, causing
  // the sampler to enter the app-limited phase.  The phase will expire by
  // itself.
  virtual void OnAppLimited() = 0;

  // Remove all the packets lower than the specified packet number.
  virtual void RemoveObsoletePackets(QuicPacketNumber least_unacked) = 0;

  // Total number of bytes sent/acked/lost in the connection.
  virtual QuicByteCount total_bytes_sent() const = 0;
  virtual QuicByteCount total_bytes_acked() const = 0;
  virtual QuicByteCount total_bytes_lost() const = 0;

  // Application-limited information exported for debugging.
  virtual bool is_app_limited() const = 0;

  virtual QuicPacketNumber end_of_app_limited_phase() const = 0;
};

// BandwidthSampler keeps track of sent and acknowledged packets and outputs a
// bandwidth sample for every packet acknowledged. The samples are taken for
// individual packets, and are not filtered; the consumer has to filter the
// bandwidth samples itself. In certain cases, the sampler will locally severely
// underestimate the bandwidth, hence a maximum filter with a size of at least
// one RTT is recommended.
//
// This class bases its samples on the slope of two curves: the number of bytes
// sent over time, and the number of bytes acknowledged as received over time.
// It produces a sample of both slopes for every packet that gets acknowledged,
// based on a slope between two points on each of the corresponding curves. Note
// that due to the packet loss, the number of bytes on each curve might get
// further and further away from each other, meaning that it is not feasible to
// compare byte values coming from different curves with each other.
//
// The obvious points for measuring slope sample are the ones corresponding to
// the packet that was just acknowledged. Let us denote them as S_1 (point at
// which the current packet was sent) and A_1 (point at which the current packet
// was acknowledged). However, taking a slope requires two points on each line,
// so estimating bandwidth requires picking a packet in the past with respect to
// which the slope is measured.
//
// For that purpose, BandwidthSampler always keeps track of the most recently
// acknowledged packet, and records it together with every outgoing packet.
// When a packet gets acknowledged (A_1), it has not only information about when
// it itself was sent (S_1), but also the information about the latest
// acknowledged packet right before it was sent (S_0 and A_0).
//
// Based on that data, send and ack rate are estimated as:
//   send_rate = (bytes(S_1) - bytes(S_0)) / (time(S_1) - time(S_0))
//   ack_rate = (bytes(A_1) - bytes(A_0)) / (time(A_1) - time(A_0))
//
// Here, the ack rate is intuitively the rate we want to treat as bandwidth.
// However, in certain cases (e.g. ack compression) the ack rate at a point may
// end up higher than the rate at which the data was originally sent, which is
// not indicative of the real bandwidth. Hence, we use the send rate as an upper
// bound, and the sample value is
//   rate_sample = min(send_rate, ack_rate)
//
// An important edge case handled by the sampler is tracking the app-limited
// samples. There are multiple meaning of "app-limited" used interchangeably,
// hence it is important to understand and to be able to distinguish between
// them.
//
// Meaning 1: connection state. The connection is said to be app-limited when
// there is no outstanding data to send. This means that certain bandwidth
// samples in the future would not be an accurate indication of the link
// capacity, and it is important to inform consumer about that. Whenever
// connection becomes app-limited, the sampler is notified via OnAppLimited()
// method.
//
// Meaning 2: a phase in the bandwidth sampler. As soon as the bandwidth
// sampler becomes notified about the connection being app-limited, it enters
// app-limited phase. In that phase, all *sent* packets are marked as
// app-limited. Note that the connection itself does not have to be
// app-limited during the app-limited phase, and in fact it will not be
// (otherwise how would it send packets?). The boolean flag below indicates
// whether the sampler is in that phase.
//
// Meaning 3: a flag on the sent packet and on the sample. If a sent packet is
// sent during the app-limited phase, the resulting sample related to the
// packet will be marked as app-limited.
//
// With the terminology issue out of the way, let us consider the question of
// what kind of situation it addresses.
//
// Consider a scenario where we first send packets 1 to 20 at a regular
// bandwidth, and then immediately run out of data. After a few seconds, we send
// packets 21 to 60, and only receive ack for 21 between sending packets 40 and
// 41. In this case, when we sample bandwidth for packets 21 to 40, the S_0/A_0
// we use to compute the slope is going to be packet 20, a few seconds apart
// from the current packet, hence the resulting estimate would be extremely low
// and not indicative of anything. Only at packet 41 the S_0/A_0 will become 21,
// meaning that the bandwidth sample would exclude the quiescence.
//
// Based on the analysis of that scenario, we implement the following rule: once
// OnAppLimited() is called, all sent packets will produce app-limited samples
// up until an ack for a packet that was sent after OnAppLimited() was called.
// Note that while the scenario above is not the only scenario when the
// connection is app-limited, the approach works in other cases too.
class BandwidthSampler : public BandwidthSamplerInterface {
 public:
  BandwidthSampler();
  ~BandwidthSampler() override;

  void OnPacketSent(ProtoTime sent_time,
                    QuicPacketNumber packet_number,
                    QuicByteCount bytes,
                    QuicByteCount bytes_in_flight,
                    HasRetransmittableData has_retransmittable_data) override;
  BandwidthSample OnPacketAcknowledged(ProtoTime ack_time,
                                       QuicPacketNumber packet_number) override;
  SendTimeState OnPacketLost(QuicPacketNumber packet_number) override;

  void OnAppLimited() override;

  void RemoveObsoletePackets(QuicPacketNumber least_unacked) override;

  QuicByteCount total_bytes_sent() const override;
  QuicByteCount total_bytes_acked() const override;
  QuicByteCount total_bytes_lost() const override;

  bool is_app_limited() const override;

  QuicPacketNumber end_of_app_limited_phase() const override;

 private:

  // ConnectionStateOnSentPacket represents the information about a sent packet
  // and the state of the connection at the moment the packet was sent,
  // specifically the information about the most recently acknowledged packet at
  // that moment.
  struct ConnectionStateOnSentPacket {
    // Time at which the packet is sent.
    ProtoTime sent_time;

    // Size of the packet.
    QuicByteCount size;

    // The value of |total_bytes_sent_at_last_acked_packet_| at the time the
    // packet was sent.
    QuicByteCount total_bytes_sent_at_last_acked_packet;

    // The value of |last_acked_packet_sent_time_| at the time the packet was
    // sent.
    ProtoTime last_acked_packet_sent_time;

    // The value of |last_acked_packet_ack_time_| at the time the packet was
    // sent.
    ProtoTime last_acked_packet_ack_time;

    // Send time states that are returned to the congestion controller when the
    // packet is acked or lost.
    SendTimeState send_time_state;

    // Snapshot constructor. Records the current state of the bandwidth
    // sampler.
    ConnectionStateOnSentPacket(ProtoTime sent_time,
                                QuicByteCount size,
                                const BandwidthSampler& sampler)
        : sent_time(sent_time),
          size(size),
          total_bytes_sent_at_last_acked_packet(
              sampler.total_bytes_sent_at_last_acked_packet_),
          last_acked_packet_sent_time(sampler.last_acked_packet_sent_time_),
          last_acked_packet_ack_time(sampler.last_acked_packet_ack_time_),
          send_time_state(sampler.is_app_limited_,
                          sampler.total_bytes_sent_,
                          sampler.total_bytes_acked_,
                          sampler.total_bytes_lost_) {}

    // Default constructor.  Required to put this structure into
    // PacketNumberIndexedQueue.
    ConnectionStateOnSentPacket()
        : sent_time(ProtoTime::Zero()),
          size(0),
          total_bytes_sent_at_last_acked_packet(0),
          last_acked_packet_sent_time(ProtoTime::Zero()),
          last_acked_packet_ack_time(ProtoTime::Zero()) {}
  };

  // Copy a subset of the (private) ConnectionStateOnSentPacket to the (public)
  // SendTimeState. Always set send_time_state->is_valid to true.
  void SentPacketToSendTimeState(const ConnectionStateOnSentPacket& sent_packet,
                                 SendTimeState* send_time_state) const;

  // The total number of congestion controlled bytes sent during the connection.
  QuicByteCount total_bytes_sent_;

  // The total number of congestion controlled bytes which were acknowledged.
  QuicByteCount total_bytes_acked_;

  // The total number of congestion controlled bytes which were lost.
  QuicByteCount total_bytes_lost_;

  // The value of |total_bytes_sent_| at the time the last acknowledged packet
  // was sent. Valid only when |last_acked_packet_sent_time_| is valid.
  QuicByteCount total_bytes_sent_at_last_acked_packet_;

  // The time at which the last acknowledged packet was sent. Set to
  // QuicTime::Zero() if no valid timestamp is available.
  ProtoTime last_acked_packet_sent_time_;

  // The time at which the most recent packet was acknowledged.
  ProtoTime last_acked_packet_ack_time_;

  // The most recently sent packet.
  QuicPacketNumber last_sent_packet_;

  // Indicates whether the bandwidth sampler is currently in an app-limited
  // phase.
  bool is_app_limited_;

  // The packet that will be acknowledged after this one will cause the sampler
  // to exit the app-limited phase.
  QuicPacketNumber end_of_app_limited_phase_;

  // Record of the connection state at the point where each packet in flight was
  // sent, indexed by the packet number.
  PacketNumberIndexedQueue<ConnectionStateOnSentPacket> connection_state_map_;

  // Handles the actual bandwidth calculations, whereas the outer method handles
  // retrieving and removing |sent_packet|.
  BandwidthSample OnPacketAcknowledgedInner(
      ProtoTime ack_time,
      QuicPacketNumber packet_number,
      const ConnectionStateOnSentPacket& sent_packet);
};
}
