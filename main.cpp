module;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
}

export module main;
export import <string>;
export import <memory>;
export import <iostream>;
export import <limits>;
export import <tuple>;
export import <memory>;
import <optional>;
import <sstream>;

static void my_avformat_close_input(AVFormatContext *av_format_context) {
  avformat_close_input(&av_format_context);
}

static void my_avcodec_free_context(AVCodecContext *av_codec_context) {
  avcodec_free_context(&av_codec_context);
}

static void my_av_packet_free(AVPacket *av_packet) {
  av_packet_free(&av_packet);
}

static void my_av_frame_free(AVFrame *av_frame) { av_frame_free(&av_frame); }

static void my_avfilter_graph_free(AVFilterGraph *filter_graph) {
  avfilter_graph_free(&filter_graph);
}

static void my_avfilter_inout_free(AVFilterInOut *filter_inout) {
  // avfilter_inout_free(&filter_inout);
}

export using MyAVFormatContext = std::shared_ptr<AVFormatContext>;
export using MyAVCodec = std::shared_ptr<const AVCodec>;
export using MyAVCodecContext = std::shared_ptr<AVCodecContext>;
export using MyAVPacket = std::shared_ptr<AVPacket>;
export using MyAVFrame = std::shared_ptr<AVFrame>;
export using MyAVFilterGraph = std::shared_ptr<AVFilterGraph>;
export using MyAVFilterInOut = std::shared_ptr<AVFilterInOut>;
export using MyAVFilterContext = std::shared_ptr<AVFilterContext>;

static MyAVFormatContext my_avformat_open_input(std::string filename) {
  AVFormatContext *av_format_context = nullptr;
  int ret = avformat_open_input(&av_format_context, filename.c_str(), nullptr,
                                nullptr);
  if (ret != 0) {
    throw std::string("avformat_open_input failed");
  }
  return MyAVFormatContext(av_format_context, &my_avformat_close_input);
}

static void my_avformat_find_stream_info(MyAVFormatContext av_format_context) {
  int ret = avformat_find_stream_info(av_format_context.get(), nullptr);
  if (ret < 0) {
    throw std::string("avformat_find_stream_info failed");
  }
}

static std::tuple<int, MyAVCodec>
my_av_find_best_stream(MyAVFormatContext av_format_context,
                       AVMediaType av_media_type) {
  const AVCodec *av_codec = nullptr;
  int ret = av_find_best_stream(av_format_context.get(), av_media_type, -1, -1,
                                &av_codec, 0);
  if (ret < 0) {
    throw std::string("av_find_best_stream failed");
  }
  return std::make_tuple(ret, MyAVCodec(av_format_context, av_codec));
}

static MyAVCodecContext my_avcodec_alloc_context3(MyAVCodec av_codec) {
  AVCodecContext *codec_context = avcodec_alloc_context3(av_codec.get());
  if (codec_context == nullptr) {
    throw std::string("avcodec_alloc_context3 failed");
  }
  return MyAVCodecContext(codec_context, &my_avcodec_free_context);
}

static void
my_avcodec_parameters_to_context(MyAVCodecContext codec_ctx,
                                 MyAVFormatContext av_format_context,
                                 int stream_index) {
  int ret = avcodec_parameters_to_context(
      codec_ctx.get(), av_format_context->streams[stream_index]->codecpar);
  if (ret < 0) {
    throw std::string("avcodec_parameters_to_context failed");
  }
}

static void my_avcodec_open2(MyAVCodecContext codec_context, MyAVCodec codec) {
  int ret = avcodec_open2(codec_context.get(), codec.get(), nullptr);
  if (ret < 0) {
    throw std::string("avcodec_open2 failed");
  }
}

static MyAVPacket my_av_packet_alloc() {
  AVPacket *packet = av_packet_alloc();
  if (packet == nullptr) {
    throw std::string("av_packet_alloc failed");
  }
  return MyAVPacket(packet, &my_av_packet_free);
}

static MyAVFrame my_av_frame_alloc() {
  AVFrame *frame = av_frame_alloc();
  if (frame == nullptr) {
    throw std::string("av_frame_alloc failed");
  }
  return MyAVFrame(frame, &my_av_frame_free);
}

static bool my_av_read_frame(MyAVFormatContext av_format_context,
                             MyAVPacket av_packet) {
  av_packet_unref(av_packet.get());
  int ret = av_read_frame(av_format_context.get(), av_packet.get());
  if (ret != 0) {
    return false;
  }
  return true;
}

static void my_avcodec_send_packet(MyAVCodecContext codec_context,
                                   MyAVPacket packet) {
  int ret = avcodec_send_packet(codec_context.get(), packet.get());
  if (ret != 0) {
    throw std::string("avcodec_send_packet failed");
  }
}

static bool my_avcodec_receive_frame(MyAVCodecContext codec_context,
                                     MyAVFrame frame) {
  int ret = avcodec_receive_frame(codec_context.get(), frame.get());
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    return false;
  } else if (ret < 0) {
    throw std::string("my_avcodec_receive_frame failed");
  } else {
    return true;
  }
}

static bool av_buffersink_get_frame(MyAVFilterContext filter_context,
                                    MyAVFrame frame) {
  av_frame_unref(frame.get());
  int ret = av_buffersink_get_frame(filter_context.get(), frame.get());
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    return false;
  } else if (ret < 0) {
    throw std::string("av_buffersink_get_frame failed");
  } else {
    return true;
  }
}

static MyAVFilterGraph my_avfilter_graph_alloc() {
  AVFilterGraph *filter_graph = avfilter_graph_alloc();
  if (filter_graph == nullptr) {
    throw std::string("avfilter_graph_alloc failed");
  }
  return MyAVFilterGraph(filter_graph, my_avfilter_graph_free);
}

static MyAVFilterInOut my_avfilter_inout_alloc(MyAVFilterGraph graph) {
  AVFilterInOut *filter_inout = avfilter_inout_alloc();
  if (filter_inout == nullptr) {
    throw std::string("avfilter_inout_alloc failed");
  }
  return MyAVFilterInOut(filter_inout, my_avfilter_inout_free);
}

static const AVFilter &my_avfilter_get_by_name(std::string name) {
  const AVFilter *filter = avfilter_get_by_name(name.c_str());
  if (filter == nullptr) {
    throw std::string("avfilter_get_by_name failed");
  }
  return *filter;
}

static MyAVFilterContext
my_avfilter_graph_create_filter(const AVFilter &filter, std::string name,
                                std::optional<std::string> args,
                                MyAVFilterGraph graph) {
  AVFilterContext *filter_context = nullptr;
  int ret = avfilter_graph_create_filter(
      &filter_context, &filter, name.c_str(),
      args.has_value() ? args->c_str() : nullptr, nullptr, graph.get());
  if (ret < 0) {
    throw std::string("my_avfilter_graph_create_filter failed");
  }
  return MyAVFilterContext(graph, filter_context);
}

static void my_avfilter_graph_parse(MyAVFilterGraph graph, std::string filters,
                                    MyAVFilterInOut inputs,
                                    MyAVFilterInOut outputs) {
  int ret = avfilter_graph_parse(graph.get(), filters.c_str(), inputs.get(),
                                 outputs.get(), nullptr);
  if (ret != 0) {
    throw std::string("avfilter_graph_parse failed");
  }
}

static void my_avfilter_graph_config(MyAVFilterGraph graph) {
  int ret = avfilter_graph_config(graph.get(), nullptr);
  if (ret < 0) {
    throw std::string("avfilter_graph_config failed");
  }
}

static std::string
my_av_channel_layout_describe_bprint(MyAVCodecContext codec_context) {
  AVBPrint bprint;
  av_bprint_init(&bprint, 0, AV_BPRINT_SIZE_UNLIMITED);

  int ret =
      av_channel_layout_describe_bprint(&codec_context->ch_layout, &bprint);
  if (ret != 0) {
    throw std::string("av_channel_layout_describe_bprint failed");
  }

  char *result = nullptr;

  av_bprint_finalize(&bprint, &result);
  if (ret != 0) {
    throw std::string("av_bprint_finalize failed");
  }

  std::string return_value = std::string(result);

  av_free(result);

  return return_value;
}

static void my_av_buffersrc_add_frame(MyAVFilterContext filter_context,
                                      MyAVFrame frame) {
  int ret = av_buffersrc_add_frame_flags(filter_context.get(), frame.get(),
                                         AV_BUFFERSRC_FLAG_KEEP_REF);
  if (ret < 0) {
    throw std::string("av_buffersrc_add_frame_flags failed");
  }
}

static std::tuple<MyAVFilterContext, MyAVFilterContext>
build_filter_tree(MyAVFormatContext format_context,
                  MyAVCodecContext audio_codec_context,
                  int audio_stream_index) {
  AVRational time_base = format_context->streams[audio_stream_index]->time_base;

  const AVFilter &abuffersrc = my_avfilter_get_by_name("abuffer");
  const AVFilter &abuffersink = my_avfilter_get_by_name("abuffersink");

  MyAVFilterGraph filter_graph = my_avfilter_graph_alloc();
  MyAVFilterInOut outputs = my_avfilter_inout_alloc(filter_graph);
  MyAVFilterInOut inputs = my_avfilter_inout_alloc(filter_graph);

  if (audio_codec_context->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
    av_channel_layout_default(&audio_codec_context->ch_layout,
                              audio_codec_context->ch_layout.nb_channels);
  }
  std::ostringstream argsstream;
  argsstream << "time_base=" << time_base.num << "/" << time_base.den
             << ":sample_rate=" << audio_codec_context->sample_rate
             << ":sample_fmt="
             << av_get_sample_fmt_name(audio_codec_context->sample_fmt)
             << ":channel_layout="
             << my_av_channel_layout_describe_bprint(audio_codec_context);
  std::string args = argsstream.str();

  MyAVFilterContext abuffersrc_ctx = my_avfilter_graph_create_filter(
      abuffersrc, "in", std::optional(args), filter_graph);

  MyAVFilterContext abuffersink_ctx = my_avfilter_graph_create_filter(
      abuffersink, "out", std::optional<std::string>(), filter_graph);

  outputs->name = av_strdup("in");
  outputs->filter_ctx = abuffersrc_ctx.get();
  outputs->pad_idx = 0;
  outputs->next = nullptr;

  inputs->name = av_strdup("out");
  inputs->filter_ctx = abuffersink_ctx.get();
  inputs->pad_idx = 0;
  inputs->next = nullptr;

  my_avfilter_graph_parse(filter_graph, "silencedetect=noise=-40dB:duration=1",
                          inputs, outputs);

  my_avfilter_graph_config(filter_graph);

  return std::make_tuple(abuffersrc_ctx, abuffersink_ctx);
}

export int main() {
  try {
    std::string filename = "file:test.mp4";
    MyAVFormatContext av_format_context = my_avformat_open_input(filename);

    my_avformat_find_stream_info(av_format_context);

    /*for (unsigned int i = 0; i < av_format_context->nb_streams; i++) {
      av_dump_format(av_format_context.get(), i, filename.c_str(), 0);
    }*/

    MyAVCodecContext audio_codec_ctx = nullptr;
    MyAVCodecContext video_codec_ctx = nullptr;

    MyAVCodec audio_codec;
    int audio_stream_index;
    std::tie(audio_stream_index, audio_codec) =
        my_av_find_best_stream(av_format_context, AVMEDIA_TYPE_AUDIO);

    MyAVCodec video_codec;
    int video_stream_index;
    std::tie(video_stream_index, video_codec) =
        my_av_find_best_stream(av_format_context, AVMEDIA_TYPE_VIDEO);

    std::cout << "audio stream: " << audio_stream_index
              << " video stream: " << video_stream_index << std::endl;

    audio_codec_ctx = my_avcodec_alloc_context3(audio_codec);
    video_codec_ctx = my_avcodec_alloc_context3(video_codec);

    my_avcodec_parameters_to_context(audio_codec_ctx, av_format_context,
                                     audio_stream_index);

    my_avcodec_parameters_to_context(video_codec_ctx, av_format_context,
                                     video_stream_index);

    my_avcodec_open2(audio_codec_ctx, audio_codec);

    my_avcodec_open2(video_codec_ctx, video_codec);

    MyAVFilterContext abuffersrc_ctx = nullptr;
    MyAVFilterContext abuffersink_ctx = nullptr;
    std::tie(abuffersrc_ctx, abuffersink_ctx) = build_filter_tree(
        av_format_context, audio_codec_ctx, audio_stream_index);

    MyAVFrame audio_filter_frame = my_av_frame_alloc();

    // AVPacketList
    MyAVPacket packet = my_av_packet_alloc();
    MyAVFrame audio_frame = my_av_frame_alloc();
    MyAVFrame video_frame = my_av_frame_alloc();

    video_codec_ctx->skip_frame = AVDiscard::AVDISCARD_NONINTRA;

    while (my_av_read_frame(av_format_context, packet)) {

      if (packet->stream_index == video_stream_index) {
        my_avcodec_send_packet(video_codec_ctx, packet);

        while (my_avcodec_receive_frame(video_codec_ctx, video_frame)) {
          std::cout << "Keyframe detected " << video_frame->key_frame << " "
                    << video_frame->pts << std::endl;
        }
      }

      // TODO FIXMe handle end of file correctly (flushing)
      if (packet->stream_index == audio_stream_index) {
        my_avcodec_send_packet(audio_codec_ctx, packet);

        while (my_avcodec_receive_frame(audio_codec_ctx, audio_frame)) {
          my_av_buffersrc_add_frame(abuffersrc_ctx, audio_frame);

          while (av_buffersink_get_frame(abuffersink_ctx, audio_filter_frame)) {

            AVDictionaryEntry *silence_start =
                av_dict_get(audio_filter_frame->metadata, "lavfi.silence_start",
                            nullptr, 0);
            AVDictionaryEntry *silence_end = av_dict_get(
                audio_filter_frame->metadata, "lavfi.silence_end", nullptr, 0);

            if (silence_start != nullptr) {
              std::cout << "silence_start: " << silence_start->value
                        << std::endl;
            }
            if (silence_end != nullptr) {
              std::cout << "silence_end: " << silence_end->value << std::endl;
            }
          }
        }
      }
    }

    // then streamcopy (or decode for partial keyframe shit)
    // https://ffmpeg.org/ffmpeg-codecs.html

    // https://ffmpeg.org/ffmpeg-filters.html#segment_002c-asegment

    // https://ffmpeg.org/ffmpeg-filters.html
    // atrim

    // https://ffmpeg.org/ffmpeg-filters.html#Timeline-editing

    // then encode

    // https://ffmpeg.org/ffmpeg-filters.html#toc-concat

    // and mux

    return 0;
  } catch (std::string error) {
    std::cerr << error << std::endl;
    return 1;
  }
}
