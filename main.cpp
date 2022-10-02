extern "C" {
    #include <libavformat/avformat.h>
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
    // TODO avformat_find_stream_info()
    // TODO av_find_best_stream
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    for (int i = 0; i < av_format_context->nb_streams; i++) {
        av_dump_format(av_format_context, i, filename, 0);
    }

    if (avformat_seek_file(av_format_context, 0, 0, 0, std::numeric_limits<int64_t>::max(), 0) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to seek\n");
        return ret;
    }
    AVPacket* av_packet = av_packet_alloc();
    if (av_read_frame(av_format_context, av_packet) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to read frame\n");
        return ret;
    }

    std::cout << "position: " << av_packet->dts << std::endl;

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
    // replaygain
    // silencedetect
    // silenceremove

    // https://ffmpeg.org/ffmpeg-filters.html#Timeline-editing

    // then encode

    // https://ffmpeg.org/ffmpeg-filters.html#toc-concat

    // and mux

    return 0;
}
