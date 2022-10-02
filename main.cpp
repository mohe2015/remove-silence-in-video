extern "C" {
    #include <libavformat/avformat.h>
}
#include <iostream>
// https://ffmpeg.org/
// https://ffmpeg.org/ffmpeg.html

int main() {
    // first use libavformat to demux the file
    // https://ffmpeg.org/ffmpeg-formats.html
    // https://ffmpeg.org/doxygen/trunk/group__libavf.html
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gac05d61a2b492ae3985c658f34622c19d
    const char    *url = "file:test.mp4";
    AVFormatContext *av_format_context = NULL;
    int ret = avformat_open_input(&av_format_context, url, NULL, NULL);
    // TODO avformat_find_stream_info()
    if (ret < 0)
        abort();

    AVPacket* av_packet = av_packet_alloc();
    while (av_read_frame(av_format_context, av_packet) == 0) {
        std::cout << "Read packet" << std::endl;
    }


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
