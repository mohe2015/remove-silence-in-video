extern "C" {
    #include <libavformat/avformat.h>
}
// https://ffmpeg.org/
// https://ffmpeg.org/ffmpeg.html

int main() {
    // first use libavformat to demux the file
    // https://ffmpeg.org/ffmpeg-formats.html
    // https://ffmpeg.org/doxygen/trunk/group__libavf.html
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gac05d61a2b492ae3985c658f34622c19d
    const char    *url = "file:in.mp3";
    AVFormatContext *s = NULL;
    int ret = avformat_open_input(&s, url, NULL, NULL);
    if (ret < 0)
        abort();


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
