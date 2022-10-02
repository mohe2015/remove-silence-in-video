module;

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersrc.h>
    #include <libavfilter/buffersink.h>
    #include <libavutil/avutil.h>
}

#include <string>
#include <memory>
#include <iostream>
#include <limits>
#include <tuple>
#include <memory>

export module lib;

export void my_avformat_close_input(AVFormatContext* av_format_context) {
    avformat_close_input(&av_format_context);
}

export std::unique_ptr<AVFormatContext, decltype(&my_avformat_close_input)> my_avformat_open_input(std::string filename) {
    AVFormatContext *av_format_context = NULL;
    int ret = avformat_open_input(&av_format_context, filename.c_str(), NULL, NULL);
    if (ret != 0) {
        throw "avformat_open_input failed";
    }
    return std::unique_ptr<AVFormatContext, decltype(&my_avformat_close_input)>(av_format_context, &my_avformat_close_input);
}