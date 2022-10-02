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

export using MyAVFormatContext = std::shared_ptr<AVFormatContext>;
export using MyAVCodec = std::shared_ptr<const AVCodec>;

export MyAVFormatContext my_avformat_open_input(std::string filename) {
  AVFormatContext *av_format_context;
  int ret =
      avformat_open_input(&av_format_context, filename.c_str(), NULL, NULL);
  if (ret != 0) {
    throw std::string("avformat_open_input failed");
  }
  return MyAVFormatContext(av_format_context, &my_avformat_close_input);
}

export void my_avformat_find_stream_info(MyAVFormatContext av_format_context) {
  int ret = avformat_find_stream_info(av_format_context.get(), NULL);
  if (ret < 0) {
    throw std::string("avformat_find_stream_info failed");
  }
}

export std::tuple<int, MyAVCodec> my_av_find_best_stream(MyAVFormatContext av_format_context, AVMediaType av_media_type) {
  const AVCodec *av_codec;
  int ret = av_find_best_stream(av_format_context.get(), av_media_type, -1, -1, &av_codec, 0);
  if (ret < 0) {
    throw std::string("av_find_best_stream failed");
  }
  return std::make_tuple(ret, MyAVCodec(av_format_context, av_codec));
}