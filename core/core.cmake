# core code
set(MP_INCLUDE_DIR
  ${MP_INCLUDE_DIR}
  ${CORE_SOURCE_DIR}/include
  ${CORE_SOURCE_DIR}/feature
  ${CORE_SOURCE_DIR}/pipeline
)
# set(MP_SOURCE_FILES
#   ${MP_SOURCE_FILES}
#   ${CORE_SOURCE_DIR}/src/media_context_impl.cpp
#   ${CORE_SOURCE_DIR}/src/media_context.cpp
#   ${CORE_SOURCE_DIR}/src/media_config_impl.cpp
#   ${CORE_SOURCE_DIR}/src/media_config.cpp
#   ${CORE_SOURCE_DIR}/src/media_packet_impl.cpp
#   ${CORE_SOURCE_DIR}/src/media_packet.cpp
#   ${CORE_SOURCE_DIR}/src/media_processor_impl.cpp
#   ${CORE_SOURCE_DIR}/src/media_processor.cpp
#   ${CORE_SOURCE_DIR}/pipeline/element_pipeline.cpp
#   ${CORE_SOURCE_DIR}/pipeline/policy_pipeline.cpp
#   ${CORE_SOURCE_DIR}/feature/feature_processor.cpp
# )

# core pipeline
# if(BUILD_PIPELINE_TEST)
#   add_subdirectory(${CORE_SOURCE_DIR}/pipeline/test)
# endif()

# feature-opus
if (BUILD_OPUS)
#   add_definitions(-DHAVE_OPUS_CONFIG_H)
  add_definitions(-DMP_INCLUDE_OPUS)
#   set(OGGOPUS_SOURCE_DIR ${ACODEC_SOURCE_DIR}/oggopus)
#   set(MP_INCLUDE_DIR
#     ${MP_INCLUDE_DIR}
#     ${OGGOPUS_SOURCE_DIR}/include
#   )
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${OGGOPUS_SOURCE_DIR}/src/oggopus_audio_in.cpp
#     ${OGGOPUS_SOURCE_DIR}/src/oggopus_encoder.cpp
#     ${OGGOPUS_SOURCE_DIR}/src/oggopus_header.cpp
#     ${OGGOPUS_SOURCE_DIR}/src/oggopus_decoder.cpp
#     ${OGGOPUS_SOURCE_DIR}/src/oggopus_utils.cpp
#     ${OGGOPUS_SOURCE_DIR}/src/lpc.cpp
#   )
  # feature
  set(MP_SOURCE_FILES
    ${MP_SOURCE_FILES}
    # ${CORE_SOURCE_DIR}/feature/opus2pcm/opus_to_pcm.cpp
    # ${CORE_SOURCE_DIR}/feature/opus2pcm/rawopus_to_pcm.cpp
    ${CORE_SOURCE_DIR}/feature/pcm2opus/pcm_to_opus.cpp
    # ${CORE_SOURCE_DIR}/feature/pcm2opus/pcm_to_rawopus.cpp
    # ${CORE_SOURCE_DIR}/feature/opus_wrappers/opus_to_rawopus.cpp
    # ${CORE_SOURCE_DIR}/feature/opus_wrappers/rawopus_to_opus.cpp
  )
#   # test utils
#   if(BUILD_OPUS_TEST)
#     add_subdirectory(${CORE_SOURCE_DIR}/feature/opus2pcm/test)
#     add_subdirectory(${CORE_SOURCE_DIR}/feature/pcm2opus/test)
#   endif()
endif ()

# feature-resample
# if (BUILD_RESAMPLE)
#   add_definitions(-DOUTSIDE_SPEEX -DRANDOM_PREFIX=NlsOpt -DFIXED_POINT)
#   set(RESAMPLER_SOURCE_DIR ${FILTER_SOURCE_DIR}/resample)
#   set(MP_INCLUDE_DIR
#     ${MP_INCLUDE_DIR}
#     ${RESAMPLER_SOURCE_DIR}/include
#   )
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${RESAMPLER_SOURCE_DIR}/src/res_arbi.cpp
#   )
#   # feature
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${CORE_SOURCE_DIR}/feature/resampler/wav_resample.cpp
#     ${CORE_SOURCE_DIR}/feature/resampler/pcm_resample.cpp
#     ${CORE_SOURCE_DIR}/feature/resampler/pcm_deinterleave.cpp
#   )
#   # test utils
#   if(BUILD_RESAMPLE_TEST)
#     add_subdirectory(${CORE_SOURCE_DIR}/feature/resampler/test)
#   endif()
# endif ()

# # feature-h264
# if (BUILD_H264)
#   set(H264_SOURCE_DIR ${VCODEC_SOURCE_DIR}/h264)
#   set(MP_INCLUDE_DIR
#     ${MP_INCLUDE_DIR}
#     ${H264_SOURCE_DIR}/include
#   )
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${H264_SOURCE_DIR}/src/h264_nal_parser.cpp
#     ${H264_SOURCE_DIR}/src/openh264_decoder.cpp
#   )
#   # feature
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${CORE_SOURCE_DIR}/feature/h2642iyuv/h264_to_iyuv.cpp
#   )
# endif ()

# # feature-jpeg
# if (BUILD_JPEG)
#   set(JPEG_SOURCE_DIR ${VCODEC_SOURCE_DIR}/jpeg)
#   set(MP_INCLUDE_DIR
#     ${MP_INCLUDE_DIR}
#     ${JPEG_SOURCE_DIR}/include
#   )
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${JPEG_SOURCE_DIR}/src/jpeg_decoder.cpp
#   )
#   # feature
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${CORE_SOURCE_DIR}/feature/iyuv2jpeg/iyuv_to_jpeg.cpp
#   )
# endif ()

# # feature-yuv-convert
# if (BUILD_YUV_CONVERT)
#   set(YUV_CONVERT_SOURCE_DIR ${FILTER_SOURCE_DIR}/scale)
#   set(MP_INCLUDE_DIR
#     ${MP_INCLUDE_DIR}
#     ${YUV_CONVERT_SOURCE_DIR}/include
#   )
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${YUV_CONVERT_SOURCE_DIR}/src/yuv_convert.cpp
#   )
#   # feature
#   set(MP_SOURCE_FILES
#     ${MP_SOURCE_FILES}
#     ${CORE_SOURCE_DIR}/feature/iyuv2iyuv/iyuv_scale.cpp
#   )
# endif ()

# # feature-copy
# set(MP_SOURCE_FILES
#   ${MP_SOURCE_FILES}
#   ${CORE_SOURCE_DIR}/feature/copy/data_copy.cpp
# )