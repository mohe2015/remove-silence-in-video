module;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

export module main;
export import <string>;
export import <memory>;
export import <iostream>;
export import <limits>;
export import <tuple>;
export import <memory>;

export void my_avformat_close_input(AVFormatContext *av_format_context) {
  avformat_close_input(&av_format_context);
}


export void my_avcodec_free_context(AVCodecContext *av_codec_context) {
  avcodec_free_context(&av_codec_context);
}

export void my_av_packet_free(AVPacket *av_packet) {
  av_packet_free(&av_packet);
}

export void my_av_frame_free(AVFrame *av_frame) {
  av_frame_free(&av_frame);
}

export using MyAVFormatContext = std::shared_ptr<AVFormatContext>;
export using MyAVCodec = std::shared_ptr<const AVCodec>;
export using MyAVCodecContext = std::shared_ptr<AVCodecContext>;
export using MyAVPacket = std::shared_ptr<AVPacket>;
export using MyAVFrame = std::shared_ptr<AVFrame>;

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

export MyAVCodecContext my_avcodec_alloc_context3(MyAVCodec av_codec) {
  AVCodecContext * codec_context = avcodec_alloc_context3(av_codec.get());
  return MyAVCodecContext(codec_context, &my_avcodec_free_context);
}

export void my_avcodec_parameters_to_context(MyAVCodecContext codec_ctx, MyAVFormatContext av_format_context, int stream_index) {
  int ret = avcodec_parameters_to_context(
        codec_ctx.get(),
        av_format_context->streams[stream_index]->codecpar);
  if (ret < 0) {
    throw std::string("avcodec_parameters_to_context failed");
  }
}

export void my_avcodec_open2(MyAVCodecContext codec_context, MyAVCodec codec) {
  int ret = avcodec_open2(codec_context.get(), codec.get(), NULL);
  if (ret < 0) {
    throw std::string("avcodec_open2 failed");
  }
}

export MyAVPacket my_av_packet_alloc() {
  AVPacket* packet = av_packet_alloc();
  return MyAVPacket(packet, &my_av_packet_free);
}

export MyAVFrame my_av_frame_alloc() {
  AVFrame* frame = av_frame_alloc();
  return MyAVFrame(frame, &my_av_frame_free);
}

export bool my_av_read_frame(MyAVFormatContext av_format_context, MyAVPacket av_packet) {
  av_packet_unref(av_packet.get());
  int ret = av_read_frame(av_format_context.get(), av_packet.get());
  if (ret != 0) {
    return false;
  }
  return true;
}

// https://ffmpeg.org/
// https://ffmpeg.org/ffmpeg.html

std::tuple<AVFilterContext *, AVFilterContext *>
build_filter_tree(AVFormatContext *format_context,
                  AVCodecContext *audio_codec_context, int audio_stream_index) {
  char args[512]; // uff
  AVRational time_base = format_context->streams[audio_stream_index]->time_base;

  const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
  const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
  AVFilterContext *abuffersrc_ctx;
  AVFilterContext *abuffersink_ctx;

  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs = avfilter_inout_alloc();

  AVFilterGraph *filter_graph = avfilter_graph_alloc();

  if (!outputs || !inputs || !filter_graph) {
    av_log(NULL, AV_LOG_ERROR, "allocation failed\n");
    exit(1);
  }

  if (audio_codec_context->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
    av_channel_layout_default(&audio_codec_context->ch_layout,
                              audio_codec_context->ch_layout.nb_channels);
  }
  int ret =
      snprintf(args, sizeof(args),
               "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",
               time_base.num, time_base.den, audio_codec_context->sample_rate,
               av_get_sample_fmt_name(audio_codec_context->sample_fmt));
  av_channel_layout_describe(&audio_codec_context->ch_layout, args + ret,
                             sizeof(args) - ret);

  ret = avfilter_graph_create_filter(&abuffersrc_ctx, abuffersrc, "in", args,
                                     NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
    exit(1);
  }

  ret = avfilter_graph_create_filter(&abuffersink_ctx, abuffersink, "out", NULL,
                                     NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create null sink\n");
    exit(1);
  }

  outputs->name = av_strdup("in");
  outputs->filter_ctx = abuffersrc_ctx;
  outputs->pad_idx = 0;
  outputs->next = NULL;

  inputs->name = av_strdup("out");
  inputs->filter_ctx = abuffersink_ctx;
  inputs->pad_idx = 0;
  inputs->next = NULL;

  if ((ret = avfilter_graph_parse_ptr(filter_graph,
                                      "silencedetect=noise=-40dB:duration=1",
                                      &inputs, &outputs, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot parse filter graph\n");
    exit(1);
  }

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot configure graph\n");
    exit(1);
  }

  return std::make_tuple(abuffersrc_ctx, abuffersink_ctx);
}

export int main() {
  try {
    int ret;
    // https://ffmpeg.org/ffmpeg-formats.html
    // https://ffmpeg.org/doxygen/trunk/group__libavf.html
    std::string filename = "file:test.mp4";
    MyAVFormatContext av_format_context = my_avformat_open_input(filename);

    my_avformat_find_stream_info(av_format_context);

    /*for (unsigned int i = 0; i < av_format_context->nb_streams; i++) {
      av_dump_format(av_format_context.get(), i, filename.c_str(), 0);
    }*/

    MyAVCodecContext audio_codec_ctx;
    MyAVCodecContext video_codec_ctx;

    // https://ffmpeg.org/doxygen/trunk/transcode_aac_8c-example.html#_a2
    // https://ffmpeg.org/doxygen/trunk/filtering_audio_8c-example.html#_a4

    // TODO FIXME maybe use same for audio and video stream
    MyAVCodec audio_codec;
    int audio_stream_index;
    std::tie(audio_stream_index, audio_codec) = my_av_find_best_stream(av_format_context, AVMEDIA_TYPE_AUDIO);

    MyAVCodec video_codec;
    int video_stream_index;
    std::tie(video_stream_index, video_codec) = my_av_find_best_stream(av_format_context, AVMEDIA_TYPE_VIDEO);

    std::cout << "audio stream: " << audio_stream_index
              << " video stream: " << video_stream_index << std::endl;

    audio_codec_ctx = my_avcodec_alloc_context3(audio_codec);
    video_codec_ctx = my_avcodec_alloc_context3(video_codec);

    my_avcodec_parameters_to_context(
        audio_codec_ctx,
        av_format_context, audio_stream_index);
   
    my_avcodec_parameters_to_context(
        video_codec_ctx,
        av_format_context, video_stream_index);
   
    my_avcodec_open2(audio_codec_ctx, audio_codec);

    my_avcodec_open2(video_codec_ctx, video_codec);

    std::cout << "works!" << std::endl;

    AVFilterContext *abuffersrc_ctx;
    AVFilterContext *abuffersink_ctx;
    std::tie(abuffersrc_ctx, abuffersink_ctx) = build_filter_tree(
        av_format_context.get(), audio_codec_ctx.get(), audio_stream_index);

    AVFrame *audio_filter_frame = av_frame_alloc();

    // AVPacketList
    MyAVPacket packet = my_av_packet_alloc();
    MyAVFrame audio_frame = my_av_frame_alloc();
    MyAVFrame video_frame = my_av_frame_alloc();

    video_codec_ctx->skip_frame = AVDiscard::AVDISCARD_NONINTRA;

    while (my_av_read_frame(av_format_context, packet)) {
      // std::cout << "Read packet" << std::endl;

      if (packet->stream_index == video_stream_index) {
        ret = avcodec_send_packet(video_codec_ctx.get(), packet.get());
        if (ret < 0) {
          av_log(NULL, AV_LOG_ERROR,
                 "Error while sending a packet to the video decoder\n");
          break;
        }

        while (ret >= 0) {
          ret = avcodec_receive_frame(video_codec_ctx.get(), video_frame.get());
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
          } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Error while receiving a frame from the decoder\n");
            return ret;
          }

          std::cout << "Keyframe detected " << video_frame->key_frame << " "
                    << video_frame->pts << std::endl;

          if (ret >= 0) {

            av_frame_unref(video_frame.get());
          }
        }
      }

      // TODO FIXMe handle end of file correctly (flushing)
      if (packet->stream_index == audio_stream_index) {
        ret = avcodec_send_packet(audio_codec_ctx.get(), packet.get());
        if (ret < 0) {
          av_log(NULL, AV_LOG_ERROR,
                 "Error while sending a packet to the audio decoder\n");
          break;
        }

        while (ret >= 0) {
          ret = avcodec_receive_frame(audio_codec_ctx.get(), audio_frame.get());
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
          } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Error while receiving a frame from the decoder\n");
            return ret;
          }

          // std::cout << "Decoded" << std::endl;

          if (ret >= 0) {
            // push the audio data from decoded frame into the filtergraph
            if (av_buffersrc_add_frame_flags(abuffersrc_ctx, audio_frame.get(),
                                             AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
              av_log(NULL, AV_LOG_ERROR,
                     "Error while feeding the audio filtergraph\n");
              break;
            }

            // pull filtered audio from the filtergraph
            while (1) {
              ret =
                  av_buffersink_get_frame(abuffersink_ctx, audio_filter_frame);
              if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
              if (ret < 0)
                exit(1);

              // ffmpeg -i test.mp4 -af "silencedetect=noise=-50dB:duration=2"
              // -f null -

              // std::cout << "entries: " << av_dict_count(filt_frame->metadata)
              // << std::endl;

              // https://ffmpeg.org/doxygen/trunk/group__lavu__dict.html
              char *buffer;
              if (av_dict_get_string(audio_filter_frame->metadata, &buffer, '=',
                                     ';') < 0) {
                av_log(NULL, AV_LOG_ERROR, "failed extracting dictionary\n");
                break;
              }
              std::cout << buffer << std::flush;

              // "lavfi.silence_start"

              av_frame_unref(audio_filter_frame);
            }
            av_frame_unref(audio_frame.get());
          }
        }
      }
    }

    // https://ffmpeg.org/doxygen/trunk/transcoding_8c-example.html

    /*
        int last_position = 0;
        int position = 0;
        while (true) {
            // maybe still read the whole file to do the audio analysis as the
       data is probably combined in the stream?

            // AVFMT_SEEK_TO_PTS

            // apparently because of b-keyframes frames would not need to be in
       order (pts vs dts) so maybe
            // this method is better than manually?
            if (avformat_seek_file(av_format_context, 0, last_position,
       position, std::numeric_limits<int64_t>::max(), 0) < 0) { av_log(NULL,
       AV_LOG_ERROR, "Failed to seek\n"); return ret;
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
