module;

// https://www.youtube.com/results?search_query=audio+video+sync+test

extern "C" {
#include <cassert>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
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
  int ret = av_interleaved_write_frame(format_context.get(), packet.get());
  if (ret < 0) {
    throw std::string("av_write_frame failed");
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

  my_avfilter_graph_parse(filter_graph, "silencedetect=noise=-40dB:duration=1",
                          inputs, outputs);

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

export int main() {
  try {
    std::string filename = "file:h9j89L8eQQk.mp4";
    std::string output_filename = "file:h9j89L8eQQk-output.mp4";
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

    AVRational audio_time_base =
        av_format_context->streams[audio_stream_index]->time_base;

    std::cout << av_format_context->iformat->name << std::endl;

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

    std::set<int64_t> keyframe_locations;
    std::map<std::pair<int64_t, int64_t>, MyAVPacket> frames;
    std::vector<std::pair<int64_t, int64_t>> silences;

    int64_t last_silence_start = 0;

    while (my_av_read_frame(av_format_context, packet)) {
      if (packet != nullptr) {
        if (packet->stream_index == video_stream_index) {
          // dts will be in order here (parsing order) which makes sense
          // but the pts is not in order
          //std::cout << "idx: " << packet->stream_index << " dts: " << packet->dts << " pts: " << packet->pts << std::endl;
        }

        MyAVPacket cloned_packet = my_av_packet_clone(packet);
        if (!frames
                 .emplace(std::make_pair(cloned_packet->pts,
                                         cloned_packet->stream_index),
                          cloned_packet)
                 .second) {
          throw std::string("duplicate");
        }
      }

      if (packet == nullptr || packet->stream_index == video_stream_index) {
        my_avcodec_send_packet(video_codec_ctx, packet);

        while (my_avcodec_receive_frame(video_codec_ctx, video_frame)) {
          // std::cout << "Keyframe detected " << video_frame->key_frame << " "
          // << video_frame->pts << std::endl;
          keyframe_locations.insert(video_frame->pts);
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
              int64_t silence_start =
                  llroundl(silence_start_double / av_q2d(audio_time_base));
              std::cout << "silence_start: " << silence_start << " detected at "
                        << audio_filter_frame->pts << std::endl;

              last_silence_start = silence_start;

              // TODO copy file from last silence end until this silence start
            }
            if (silence_end != nullptr) {
              // this conversion is terrible
              long double silence_end_double =
                  std::stod(std::string(silence_end->value));
              int64_t silence_end =
                  llroundl(silence_end_double / av_q2d(audio_time_base));
              std::cout << "silence_end: " << silence_end << " detected at "
                        << audio_filter_frame->pts << std::endl;

              silences.emplace_back(last_silence_start, silence_end);

              // we probably can't immediately start rendering here because we
              // currently have rounding bugs

              // render file from last keyframe to this silence end, then write
              // keyframe. maybe the keyframe could be before the last
              // silence_start?
            }

            // the problem is the silence start is sent later so we can't use
            // this at all std::cout << "audio filtered until: " <<
            // audio_filter_frame->pts << std::endl;

            // TODO FIXME improve the filter impl that it tells you until where
            // it analyzed and returns time as integer maybe also let it send
            // start and end in the end packet
          }
        }
      }
    }

    int64_t rendered_until = 0;
    int64_t dts_difference = 0;
    int64_t pts_difference = 0;
    for (auto silence : silences) {
      std::cout << "handling silence: " << silence.first << " - "
                << silence.second << std::endl;

      std::vector<std::pair<std::pair<int64_t, int64_t>, MyAVPacket>> sorted(frames.lower_bound(std::make_pair(rendered_until, 0)), frames.upper_bound(std::make_pair(silence.first, 2/*TODO FIXME this is wrong*/)));
      std::sort(sorted.begin(), sorted.end(), 
          [](std::pair<std::pair<int64_t, int64_t>, MyAVPacket> a, std::pair<std::pair<int64_t, int64_t>, MyAVPacket> b) { return b.second->dts > a.second->dts; });

      // copy from last until silence_start
      for (auto p : sorted) {
        if (p.second->stream_index == audio_stream_index) {
          MyAVPacket packet = my_av_packet_clone(p.second);
      
          packet->pos = -1;
          packet->stream_index = 0;
          //packet->dts -= dts_difference;
          //packet->pts -= pts_difference;
/*
    av_packet_rescale_ts(
              packet.get(),
              av_format_context->streams[audio_stream_index]->time_base,
              output_audio_stream->time_base);
              */

          my_av_interleaved_write_frame(output_format_context, packet);
        }

        if (p.second->stream_index == video_stream_index) {
          MyAVPacket packet = my_av_packet_clone(p.second);
          packet->pos = -1;
          packet->stream_index = 1;
          //packet->dts -= dts_difference;
          //packet->pts -= pts_difference;

          //std::cout << "dts: " << packet->dts << " pts: " << packet->pts << std::endl;
/*
          av_packet_rescale_ts(
              packet.get(),
              av_format_context->streams[video_stream_index]->time_base,
              output_video_stream->time_base);
*/
         // std::cout << "dts; " << packet->dts << " pts; " << packet->pts << std::endl;

          // packets are out of order bruh

          // dts need to be in order
          my_av_interleaved_write_frame(output_format_context, packet);
        }
      }

      // to create keyframe at silence_end we need to go from last keyframe
      // before silence_end to silence_end
      rendered_until = silence.second;
      pts_difference += silence.second - silence.first; // hacky
      dts_difference += silence.second - silence.first; // hacky
    }

    my_av_write_trailer(output_format_context);

    // we would need to seek in the input file to the keyframe (which should be
    // fast) then we decode from there on until the place we need a keyframe of
    // then we encode that keyframe
    // then we copy the rest of the input file

    // maybe we simply cache the raw packets since the last keyframe and since
    // the last audio filter response (for now maybe just cache everything?) for
    // now just cache the whole file

    return 0;
  } catch (std::string error) {
    std::cerr << error << std::endl;
    return 1;
  }
}
