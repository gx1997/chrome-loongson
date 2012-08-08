# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'webkit_media',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/media/media.gyp:yuv_convert',
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'sources': [
        'android/audio_decoder_android.cc',
        'android/webmediaplayer_android.cc',
        'android/webmediaplayer_android.h',
        'android/webmediaplayer_proxy_android.cc',
        'android/webmediaplayer_proxy_android.h',
        'active_loader.cc',
        'active_loader.h',
        'audio_decoder.cc',
        'audio_decoder.h',
        'buffered_data_source.cc',
        'buffered_data_source.h',
        'buffered_resource_loader.cc',
        'buffered_resource_loader.h',
        'filter_helpers.cc',
        'filter_helpers.h',
        'key_systems.cc',
        'key_systems.h',
        'media_stream_client.h',
        'preload.h',
        'skcanvas_video_renderer.cc',
        'skcanvas_video_renderer.h',
        'webmediaplayer_delegate.h',
        'webmediaplayer_impl.cc',
        'webmediaplayer_impl.h',
        'webmediaplayer_proxy.cc',
        'webmediaplayer_proxy.h',
        'webmediaplayer_util.cc',
        'webmediaplayer_util.h',
        'webvideoframe_impl.cc',
        'webvideoframe_impl.h',
      ],
      'conditions': [
        ['inside_chromium_build==0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
        ['OS == "android"', {
          'sources!': [
            'audio_decoder.cc',
            'webmediaplayer_impl.cc',
            'webmediaplayer_impl.h',
          ],
          'dependencies': [
            '<(DEPTH)/media/media.gyp:player_android',
          ],
        }, { # OS != "android"'
          'sources/': [
            ['exclude', '^android/'],
          ],
        }],
      ],
    },
  ],
}
