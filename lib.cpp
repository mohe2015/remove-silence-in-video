module;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

export module lib;
import <string>;
import <memory>;
import <iostream>;
import <limits>;
import <tuple>;
import <memory>;

export void my_avformat_close_input(AVFormatContext *av_format_context) {
  avformat_close_input(&av_format_context);
}

export std::unique_ptr<AVFormatContext, decltype(&my_avformat_close_input)>
my_avformat_open_input(std::string filename) {
  AVFormatContext *av_format_context = NULL;
  int ret =
      avformat_open_input(&av_format_context, filename.c_str(), NULL, NULL);
  if (ret != 0) {
    throw std::string("avformat_open_input failed");
  }
  return std::unique_ptr<AVFormatContext, decltype(&my_avformat_close_input)>(
      av_format_context, &my_avformat_close_input);
}