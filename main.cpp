extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}
#include <iostream>
#include <limits>
// https://ffmpeg.org/
// https://ffmpeg.org/ffmpeg.html

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

    /*
    // AVPacketList
    AVPacket* av_packet = av_packet_alloc();
    while (av_read_frame(av_format_context, av_packet) == 0) {
        //std::cout << "Read packet" << std::endl;


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

        av_packet_unref(av_packet);
    }*/


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
