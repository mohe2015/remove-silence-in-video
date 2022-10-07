module;

extern "C" {
#include <cassert>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <libavutil/imgutils.h>
}

export module main;
import <string>;
import <memory>;
import <iostream>;
import <limits>;
import <tuple>;
import <memory>;
import <optional>;
import <sstream>;
import <set>;
import <map>;
import <cmath>;
import <vector>;
import <utility>;
import <thread>;

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
export using MyAVStream = std::shared_ptr<AVStream>;
export using MyAVIOContext = std::shared_ptr<AVIOContext>;

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
                             MyAVPacket &av_packet) {
  if (av_packet.get() == nullptr) {
    std::cout << "ALREADY FLUSHED, EXITING" << std::endl;
    return false;
  }
  av_packet_unref(av_packet.get());
  int ret = av_read_frame(av_format_context.get(), av_packet.get());
  if (ret == AVERROR_EOF) {
    std::cout << "DETECTED EOF, FLUSHING" << std::endl;
    av_packet.reset();
    return true;
  }
  if (ret != 0) {
    throw std::string("av_read_frame failed");
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

static void my_avcodec_send_frame(MyAVCodecContext codec_context,
                                  MyAVFrame frame) {
  int ret = avcodec_send_frame(codec_context.get(), frame.get());
  if (ret != 0) {
    throw std::string("my_avcodec_send_frame failed " + std::to_string(ret));
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

static bool my_avcodec_receive_packet(MyAVCodecContext codec_context,
                                      MyAVPacket packet) {
  int ret = avcodec_receive_packet(codec_context.get(), packet.get());
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    return false;
  } else if (ret < 0) {
    throw std::string("avcodec_receive_packet failed");
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

static MyAVFormatContext my_avformat_alloc_context() {
  AVFormatContext *format_context = avformat_alloc_context();
  if (format_context == nullptr) {
    throw std::string("avformat_alloc_context failed");
  }
  return MyAVFormatContext(format_context, &avformat_free_context);
}

static MyAVFormatContext
my_avformat_alloc_output_context2(std::string format_name) {
  AVFormatContext *format_context;
  int ret = avformat_alloc_output_context2(&format_context, nullptr,
                                           format_name.c_str(), nullptr);
  if (ret < 0) {
    throw std::string("avformat_alloc_output_context2 failed");
  }
  return MyAVFormatContext(format_context, &avformat_free_context);
}

static MyAVStream my_avformat_new_stream(MyAVFormatContext format_context) {
  AVStream *stream = avformat_new_stream(format_context.get(), nullptr);
  if (stream == nullptr) {
    throw std::string("avformat_new_stream failed");
  }
  return MyAVStream(format_context, stream);
}

static MyAVIOContext my_avio_open(std::string url) {
  // TODO FIXME lifetime shorter than MyAVFormatContext
  AVIOContext *io_context;
  int ret = avio_open(&io_context, url.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    throw std::string("avio_open failed");
  }
  return MyAVIOContext(io_context, avio_close);
}

static void my_avcodec_parameters_copy(AVCodecParameters *destination,
                                       const AVCodecParameters *source) {
  int ret = avcodec_parameters_copy(destination, source);
  if (ret < 0) {
    throw std::string("avcodec_parameters_copy failed");
  }
}

static void my_avformat_write_header(MyAVFormatContext format_context) {
  int ret = avformat_write_header(format_context.get(), nullptr);
  if (ret < 0) {
    throw std::string("avformat_write_header failed");
  }
}

static void my_av_interleaved_write_frame(MyAVFormatContext format_context,
                                          MyAVPacket packet) {
  // std::cout << "dts: " << packet->dts << " pts: " << packet->pts <<
  // std::endl;

  int ret = av_interleaved_write_frame(format_context.get(), packet.get());
  if (ret < 0) {
    throw std::string("av_interleaved_write_frame failed");
  }
}

static void my_av_write_trailer(MyAVFormatContext format_context) {
  int ret = av_write_trailer(format_context.get());
  if (ret != 0) {
    throw std::string("av_write_trailer failed");
  }
}

static MyAVPacket my_av_packet_clone(MyAVPacket packet) {
  AVPacket *cloned_packet = av_packet_clone(packet.get());
  if (cloned_packet == nullptr) {
    throw std::string("av_packet_alloc failed");
  }
  return MyAVPacket(cloned_packet, &my_av_packet_free);
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

  my_avfilter_graph_parse(
      filter_graph, "silencedetect=noise=-25dB:duration=0.25", inputs, outputs);

  my_avfilter_graph_config(filter_graph);

  return std::make_tuple(abuffersrc_ctx, abuffersink_ctx);
}

static std::tuple<int, MyAVCodecContext>
get_decoder(MyAVFormatContext format_context, AVMediaType media_type) {
  MyAVCodecContext codec_context = nullptr;

  MyAVCodec codec;
  int stream_index;
  std::tie(stream_index, codec) =
      my_av_find_best_stream(format_context, media_type);

  codec_context = my_avcodec_alloc_context3(codec);

  my_avcodec_parameters_to_context(codec_context, format_context, stream_index);

  my_avcodec_open2(codec_context, codec);

  return std::make_tuple(stream_index, codec_context);
}

static MyAVCodec my_avcodec_find_encoder(MyAVCodecContext format_context) {
  const AVCodec *codec = avcodec_find_encoder(format_context->codec_id);
  if (codec == nullptr) {
    throw std::string("avcodec_find_encoder failed");
  }
  return MyAVCodec(format_context, codec);
}

export int main() {
  try {
    av_log_set_level(AV_LOG_ERROR);

    std::string filename = "file:c1_2.mp4";
    std::string output_filename = "file:c1_2-output.mp4";
    std::string format = "mp4";

    MyAVFormatContext av_format_context = my_avformat_open_input(filename);

    my_avformat_find_stream_info(av_format_context);

    av_dump_format(av_format_context.get(), 0, filename.c_str(), 0);

    MyAVCodecContext audio_codec_ctx = nullptr;
    int audio_stream_index;
    std::tie(audio_stream_index, audio_codec_ctx) =
        get_decoder(av_format_context, AVMEDIA_TYPE_AUDIO);

    MyAVCodecContext video_codec_ctx = nullptr;
    int video_stream_index;
    std::tie(video_stream_index, video_codec_ctx) =
        get_decoder(av_format_context, AVMEDIA_TYPE_VIDEO);

    std::cout << "video stream index: " << video_stream_index
              << " audio stream index: " << audio_stream_index << std::endl;

    MyAVFilterContext abuffersrc_ctx = nullptr;
    MyAVFilterContext abuffersink_ctx = nullptr;
    std::tie(abuffersrc_ctx, abuffersink_ctx) = build_filter_tree(
        av_format_context, audio_codec_ctx, audio_stream_index);

    MyAVFrame audio_filter_frame = my_av_frame_alloc();

    MyAVPacket packet = my_av_packet_alloc();
    MyAVFrame audio_frame = my_av_frame_alloc();
    MyAVFrame video_frame = my_av_frame_alloc();

    video_codec_ctx->skip_frame = AVDiscard::AVDISCARD_NONINTRA;

    MyAVFormatContext output_format_context =
        my_avformat_alloc_output_context2(format.c_str());

    MyAVIOContext output_io_context = my_avio_open(output_filename.c_str());

    if (output_format_context->oformat->flags & AVFMT_NOFILE) {
      throw new std::string("AVFMT_NOFILE");
    }

    output_format_context->pb = output_io_context.get();

    MyAVStream output_audio_stream =
        my_avformat_new_stream(output_format_context);
    my_avcodec_parameters_copy(
        output_audio_stream->codecpar,
        av_format_context->streams[audio_stream_index]->codecpar);
    output_audio_stream->codecpar->codec_tag = 0;

    MyAVStream output_video_stream =
        my_avformat_new_stream(output_format_context);
    my_avcodec_parameters_copy(
        output_video_stream->codecpar,
        av_format_context->streams[video_stream_index]->codecpar);
    output_video_stream->codecpar->codec_tag = 0;

    av_dump_format(output_format_context.get(), 0, output_filename.c_str(), 1);

    my_avformat_write_header(output_format_context);

    std::set<double> keyframe_locations;
    std::map<std::pair<double, int64_t>, MyAVPacket> frames;
    std::vector<std::pair<double, double>> silences;

    double last_silence_start = 0;

    while (my_av_read_frame(av_format_context, packet)) {
      if (packet != nullptr) {
        MyAVPacket cloned_packet = my_av_packet_clone(packet);
        if (!frames
                 .emplace(std::make_pair(
                              cloned_packet->pts *
                                  av_q2d(av_format_context
                                             ->streams[packet->stream_index]
                                             ->time_base),
                              cloned_packet->stream_index),
                          cloned_packet)
                 .second) {
          throw std::string("duplicate");
        }
      }

      if (packet == nullptr || packet->stream_index == video_stream_index) {
        my_avcodec_send_packet(video_codec_ctx, packet);

        while (my_avcodec_receive_frame(video_codec_ctx, video_frame)) {
          keyframe_locations.insert(
              av_q2d(
                  av_format_context->streams[video_stream_index]->time_base) *
              video_frame->pts);
        }
      }

      if (packet == nullptr || packet->stream_index == audio_stream_index) {
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
              long double silence_start_double =
                  std::stod(std::string(silence_start->value));

              last_silence_start = silence_start_double;
            }
            if (silence_end != nullptr) {
              long double silence_end_double =
                  std::stod(std::string(silence_end->value));

              silences.emplace_back(last_silence_start, silence_end_double);
            }
            // TODO FIXME improve the filter impl that it tells you until where
            // it analyzed and returns time as rational
          }
        }
      }
    }

    video_codec_ctx->skip_frame = AVDiscard::AVDISCARD_NONE;

    double rendered_until = 0;
    double dts_difference = 0;
    double pts_difference = 0;
    for (auto silence : silences) {
      const MyAVCodec video_encoder = my_avcodec_find_encoder(video_codec_ctx);

      MyAVCodecContext video_encoding_context =
          my_avcodec_alloc_context3(video_encoder);

      video_encoding_context->height = video_codec_ctx->height;
      video_encoding_context->width = video_codec_ctx->width;
      video_encoding_context->sample_aspect_ratio =
          video_codec_ctx->sample_aspect_ratio;
      if (video_encoder->pix_fmts)
        video_encoding_context->pix_fmt = video_encoder->pix_fmts[0];
      else
        video_encoding_context->pix_fmt = video_codec_ctx->pix_fmt;
      video_encoding_context->time_base = video_codec_ctx->time_base;

      if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        video_encoding_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

      my_avcodec_open2(video_encoding_context, video_encoder);

      std::cout << "silence " << silence.first << "-" << silence.second
                << std::endl;

      // copy all frames from last rendered until start of this silence
      std::vector<std::pair<std::pair<double, int64_t>, MyAVPacket>> sorted(
          frames.lower_bound(std::make_pair(
              rendered_until, std::numeric_limits<int64_t>::min())),
          frames.upper_bound(std::make_pair(
              silence.first, std::numeric_limits<int64_t>::max())));

      for (auto p : sorted) {
        if (p.second->stream_index == audio_stream_index) {
          MyAVPacket packet = my_av_packet_clone(p.second);

          packet->pos = -1;
          packet->stream_index = 0;

          packet->pts -=
              llroundl(pts_difference /
                       av_q2d(av_format_context->streams[audio_stream_index]
                                  ->time_base)) -
              1;
          packet->dts = packet->pts;

          av_packet_rescale_ts(
              packet.get(),
              av_format_context->streams[audio_stream_index]->time_base,
              output_audio_stream->time_base);

          my_av_interleaved_write_frame(output_format_context, packet);
        }

        if (p.second->stream_index == video_stream_index) {
          MyAVPacket packet = my_av_packet_clone(p.second);
          packet->pos = -1;
          packet->stream_index = 1;

          packet->dts -= llroundl(
              dts_difference /
              av_q2d(
                  av_format_context->streams[video_stream_index]->time_base));
          packet->pts -= llroundl(
              pts_difference /
              av_q2d(
                  av_format_context->streams[video_stream_index]->time_base));
          packet->dts = packet->pts;

          av_packet_rescale_ts(
              packet.get(),
              av_format_context->streams[video_stream_index]->time_base,
              output_video_stream->time_base);

          my_av_interleaved_write_frame(output_format_context, packet);
        }
      }

      rendered_until = silence.second;

      pts_difference += silence.second - silence.first - 0.04;

      // to create keyframe at silence_end we need to go from last keyframe
      // before silence_end to silence_end
      auto keyframe_it = std::lower_bound(
          keyframe_locations.rbegin(), keyframe_locations.rend(),
          silence.second, std::greater<double>());
      if (keyframe_it == keyframe_locations.rend()) {
        throw std::string("no keyframe found!");
      }

      double last_keyframe = *keyframe_it;
      double frame_we_need = silence.second;

      std::cout << "keyframe generate from range: " << last_keyframe << "-"
                << frame_we_need << std::endl;

      std::vector<std::pair<std::pair<double, int64_t>, MyAVPacket>>
          sorted_keyframe_gen(
              frames.lower_bound(std::make_pair(
                  last_keyframe, std::numeric_limits<int64_t>::min())),
              frames.upper_bound(std::make_pair(
                  frame_we_need, std::numeric_limits<int64_t>::max())));

      std::optional<MyAVFrame> last_video_frame;
      MyAVFrame last_audio_frame;
      avcodec_flush_buffers(video_codec_ctx.get());
      for (auto p : sorted_keyframe_gen) {
        if (p.second->stream_index == video_stream_index) {
          MyAVPacket packet = my_av_packet_clone(p.second);

          my_avcodec_send_packet(video_codec_ctx, packet);

          while (my_avcodec_receive_frame(
              video_codec_ctx, video_frame)) { // another function that randomly
                                               // calls unref on the frame
            last_video_frame =
                MyAVFrame(av_frame_clone(video_frame.get()), my_av_frame_free);
          }
        }
      }

      if (!last_video_frame.has_value()) {
        throw std::string("no last video frame found");
      }

      my_avcodec_send_frame(video_encoding_context, last_video_frame.value());
      my_avcodec_send_frame(video_encoding_context, nullptr);

      MyAVPacket video_packet = my_av_packet_alloc();
      while (my_avcodec_receive_packet(video_encoding_context, video_packet)) {
        video_packet->pos = -1;
        video_packet->stream_index = 1;

        video_packet->pts -= llroundl(
            pts_difference /
            av_q2d(av_format_context->streams[video_stream_index]->time_base));
        video_packet->dts = video_packet->pts;

        av_packet_rescale_ts(
            video_packet.get(),
            av_format_context->streams[video_stream_index]->time_base,
            output_video_stream->time_base);

        my_av_interleaved_write_frame(output_format_context, video_packet);
      }
    }

    my_av_write_trailer(output_format_context);

    return 0;
  } catch (std::string error) {
    std::cerr << error << std::endl;
    return 1;
  }
}
