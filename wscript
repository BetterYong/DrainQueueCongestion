# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
def build(bld):
    module = bld.create_ns3_module('dqc', ['network','internet','core'])
    module.source = [
    'model/thirdparty/src/ack_frame.cc',
    'model/thirdparty/src/alarm.cc',
    'model/thirdparty/src/byte_codec.cc',
    'model/thirdparty/src/flag_impl.cc',
    'model/thirdparty/src/ip_address.cc',
    'model/thirdparty/src/memslice.cc',
    'model/thirdparty/src/netutils.cc',
    'model/thirdparty/src/packet_number.cc',
    'model/thirdparty/src/platform_linux.c',
    'model/thirdparty/src/platform_windows.c',
    'model/thirdparty/src/process_alarm_factory.cc',
    'model/thirdparty/src/proto_bandwidth.cc',
    'model/thirdparty/src/proto_con.cc',
    'model/thirdparty/src/proto_error_codes.cc',
    'model/thirdparty/src/proto_framer.cc',
    'model/thirdparty/src/proto_socket.cc',
    'model/thirdparty/src/proto_stream.cc',
    'model/thirdparty/src/proto_stream_sequencer.cc',
    'model/thirdparty/src/proto_time.cc',
    'model/thirdparty/src/proto_types.cc',
    'model/thirdparty/src/proto_utils.cc',
    'model/thirdparty/src/random.cc',
    'model/thirdparty/src/received_packet_manager.cc',
    'model/thirdparty/src/rtt_stats.cc',
    'model/thirdparty/src/send_packet_manager.cc',
    'model/thirdparty/src/send_receive.cc',
    'model/thirdparty/src/socket_address.cc',
    'model/thirdparty/src/string_utils.cc',
    'model/thirdparty/src/unacked_packet_map.cc',
    'model/thirdparty/congestion/pcc_monitor_interval_queue.cc',
    'model/thirdparty/congestion/pcc_sender.cc',
    'model/thirdparty/congestion/proto_bandwidth_sampler.cc',
    'model/thirdparty/congestion/proto_bbr_sender.cc',
    'model/thirdparty/congestion/proto_bbr_sender_old.cc', 
    'model/thirdparty/congestion/proto_bbr_sender_v0.cc',  
    'model/thirdparty/congestion/proto_pacing_sender.cc',
    'model/thirdparty/congestion/proto_send_algorithm_interface.cc',
    'model/dqc_delay_ack_receiver.cc',
    'model/dqc_sender.cc',
    'model/dqc_receiver.cc',
    'model/global-stream.cc',
    'model/time_tag.cc',
    'model/dqc_trace.cc',
    ]
    headers = bld(features='ns3header')
    headers.module = 'dqc'
    headers.source = [
    'model/dqc_delay_ack_receiver.h',
    'model/dqc_sender.h',
    'model/dqc_receiver.h',
    'model/dqc_clock.h',
    'model/global-stream.h',
    'model/time_tag.h',
    'model/dqc_trace.h',
    ]
    module.env.append_value('DEFINES', '__STDC_FORMAT_MACROS')
    module.env.append_value("CFLAGS","-fPIC")
    module.env['INCLUDES'] += [
    'model',
    'model/thirdparty/congestion',
    'model/thirdparty/include',
    'model/thirdparty/logging',
		           ]
