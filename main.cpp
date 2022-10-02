extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavfilter/avfilter.h>
}
#include <iostream>
#include <limits>
// https://ffmpeg.org/
// https://ffmpeg.org/ffmpeg.html

int build_filter_tree(AVFormatContext *format_context, AVCodecContext *audio_codec_context, int audio_stream_index) {
    char args[512]; // uff
    AVRational time_base = format_context->streams[audio_stream_index]->time_base;

    //const AVFilter* silencedetect = avfilter_get_by_name("silencedetect");
    const AVFilter* abuffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    const AVFilter *null_filter = avfilter_get_by_name("null");
    AVFilterContext *null_filter_context;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    //AVFilterContext *silencedetect_context;

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    AVFilterGraph * filter_graph = avfilter_graph_alloc();

     if (!outputs || !inputs || !filter_graph) {
        return AVERROR(ENOMEM);
    }

     if (audio_codec_context->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&audio_codec_context->ch_layout, audio_codec_context->ch_layout.nb_channels);
     }
    int ret = snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",
             time_base.num, time_base.den, audio_codec_context->sample_rate,
             av_get_sample_fmt_name(audio_codec_context->sample_fmt));
    av_channel_layout_describe(&audio_codec_context->ch_layout, args + ret, sizeof(args) - ret);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        return ret;
    }

    ret = avfilter_graph_create_filter(&null_filter_context, null_filter, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create null sink\n");
        return ret;
    }
 
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
 
    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = null_filter_context;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
 
    if ((ret = avfilter_graph_parse_ptr(filter_graph, "silencedetect",
                                        &inputs, &outputs, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot parse filter graph\n");
        return ret;
    }
 
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot configure graph\n");
        return ret;
    }

    return 0;
}

int main() {
    // first use libavformat to demux the file
    // https://ffmpeg.org/ffmpeg-formats.html
    // https://ffmpeg.org/doxygen/trunk/group__libavf.html
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gac05d61a2b492ae3985c658f34622c19d
    const char    *filename = "file:test.mp4";
    AVFormatContext *av_format_context = NULL;
    int ret = avformat_open_input(&av_format_context, filename, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

     if ((ret = avformat_find_stream_info(av_format_context, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (int i = 0; i < av_format_context->nb_streams; i++) {
        av_dump_format(av_format_context, i, filename, 0);
    }

    const AVCodec *audio_codec;
    AVCodecContext *audio_codec_ctx;

    // https://ffmpeg.org/doxygen/trunk/transcode_aac_8c-example.html#_a2
    // https://ffmpeg.org/doxygen/trunk/filtering_audio_8c-example.html#_a4

    // TODO FIXME maybe use same for audio and video stream
    ret = av_find_best_stream(av_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in the input file\n");
        return ret;
    }
    int audio_stream_index = ret;
    
    audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_ctx) {
        av_log(NULL, AV_LOG_ERROR, "could not allocate coded context\n");
        return ret;
    }

    ret = avcodec_parameters_to_context(audio_codec_ctx, av_format_context->streams[audio_stream_index]->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "failed to add parameters to context\n");
        return ret;
    }
 
    if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "could not open decoder\n");
        return ret;
    }

    std::cout << "works!" << std::endl;

    if (build_filter_tree(av_format_context, audio_codec_ctx, audio_stream_index) != 0) {
        std::cout << "failed to build filter tree!" << std::endl;
        return 1;
    }

    return 0;

     // AVPacketList
    AVPacket* packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    while (av_read_frame(av_format_context, packet) == 0) {
        std::cout << "Read packet" << std::endl;

        // TODO FIXMe handle end of file correctly (flushing)
        if (packet->stream_index == audio_stream_index) {
            ret = avcodec_send_packet(audio_codec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    return ret;
                }

                std::cout << "Decoded" << std::endl;
 
                if (ret >= 0) {
                    // push the audio data from decoded frame into the filtergraph
                    /*if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
                        break;
                    }
 
                    // pull filtered audio from the filtergraph 
                    while (1) {
                        ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        if (ret < 0)
                            goto end;
                        print_frame(filt_frame);
                        av_frame_unref(filt_frame);
                    }*/
                    av_frame_unref(frame);
                }
            }
        }
    }

        // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaa23f7619d8d4ea0857065d9979c75ac8

        // I think to properly get the locations of keyframes etc we need to read the whole file anyways?

        // av_seek_frame
        // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaa23f7619d8d4ea0857065d9979c75ac8
        
        // find I-Frames

        // I think the skip frames option is the most efficient way?

        // https://ffmpeg.org/doxygen/trunk/structAVCodecParserContext.html#ac115e048335e4a7f1d85541cebcf2013
        // https://ffmpeg.org/doxygen/trunk/structAVIndexEntry.html
        // https://ffmpeg.org/doxygen/trunk/group__lavu__picture.html#gae6cbcab1f70d8e476757f1c1f5a0a78e

        // https://ffmpeg.org/doxygen/trunk/group__lavc__encdec.html
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#gga352363bce7d3ed82c101b3bc001d1c16aabee31ca5c7c140d3a84b848164eeaf8
        // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga3b40fc8d2fda6992ae6ea2567d71ba30
        // http://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga75603d7c2b8adf5829f4fd2fb860168f
        // http://ffmpeg.org/doxygen/trunk/avformat_8h.html#a23159bdc0b27ccf964072e30d6cc4559

    av_packet_unref(packet);

    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html


    // https://ffmpeg.org/doxygen/trunk/structSilenceDetectContext.html#a8a837af9608233d8988f7ac8f867c584
    // silencedetect

    // libavfilter
    // https://ffmpeg.org/doxygen/trunk/transcoding_8c-example.html#a110
    // https://ffmpeg.org/doxygen/trunk/filtering_audio_8c-example.html#a58

    // avcodec_find_decoder
    // avcodec_send_packet

/*
    int last_position = 0;
    int position = 0;
    while (true) {
        // maybe still read the whole file to do the audio analysis as the data is probably combined in the stream?

        // AVFMT_SEEK_TO_PTS

        // apparently because of b-keyframes frames would not need to be in order (pts vs dts) so maybe
        // this method is better than manually?
        if (avformat_seek_file(av_format_context, 0, last_position, position, std::numeric_limits<int64_t>::max(), 0) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to seek\n");
            return ret;
        }
        AVPacket* av_packet = av_packet_alloc();
        if (av_read_frame(av_format_context, av_packet) != 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to read frame\n");
            return ret;
        }

        std::cout << "position: " << av_packet->dts << std::endl;
        last_position = position + 1;
        position = av_packet->dts + 1;
    }
*/

    avformat_close_input(&av_format_context);

    // then streamcopy (or decode for partial keyframe shit)
    // https://ffmpeg.org/ffmpeg-codecs.html
    // the problem with partial keyframe stuff is we probably need to find all keyframes once ahead
    // to know from where on we need to reencode. maybe in that pass also do audio analysis
    // probably just store the keyframe locations to use in second pass?

    // https://ffmpeg.org/ffmpeg-filters.html#segment_002c-asegment

    // https://ffmpeg.org/ffmpeg-filters.html
    // aspectralstats
    // astats
    // atrim
    // silencedetect

    // https://ffmpeg.org/ffmpeg-filters.html#Timeline-editing

    // then encode

    // https://ffmpeg.org/ffmpeg-filters.html#toc-concat

    // and mux

    return 0;
}
