// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"

#include <stddef.h>

#ifndef GL_FALSE
#define GL_FALSE 0
#endif  // GL_FALSE

#ifndef GL_TRUE
#define GL_TRUE 1
#endif  // GL_TRUE

#if defined(__GNUC__) && !defined(__APPLE__)
#define PP_TLS __thread
#elif defined(_MSC_VER)
#define PP_TLS __declspec(thread)
#else
// TODO(alokp): Fix all other platforms.
#define PP_TLS
#endif

// TODO(alokp): This will need to be thread-safe if we build gles2 as a
// shared library.
static const struct PPB_OpenGLES2* g_gles2_interface = NULL;
static const struct PPB_OpenGLES2InstancedArrays_Dev*
    g_gles2_instanced_arrays_interface = NULL;
static const struct PPB_OpenGLES2FramebufferBlit_Dev*
    g_gles2_framebuffer_blit_interface = NULL;
static const struct PPB_OpenGLES2FramebufferMultisample_Dev*
    g_gles2_framebuffer_multisample_interface = NULL;
static const struct PPB_OpenGLES2ChromiumEnableFeature_Dev*
    g_gles2_chromium_enable_feature_interface = NULL;
static const struct PPB_OpenGLES2ChromiumMapSub_Dev*
    g_gles2_chromium_map_sub_interface = NULL;
static const struct PPB_OpenGLES2Query_Dev*
    g_gles2_query_interface = NULL;

// TODO(alokp): Make sure PP_TLS works on all supported platforms.
static PP_TLS PP_Resource g_current_context = 0;

GLboolean GL_APIENTRY glInitializePPAPI(
    PPB_GetInterface get_browser_interface) {
  if (!g_gles2_interface) {
    g_gles2_interface = get_browser_interface(PPB_OPENGLES2_INTERFACE);
  }
  if (!g_gles2_instanced_arrays_interface) {
    g_gles2_instanced_arrays_interface =
        get_browser_interface(PPB_OPENGLES2_INSTANCEDARRAYS_DEV_INTERFACE);
  }
  if (!g_gles2_framebuffer_blit_interface) {
    g_gles2_framebuffer_blit_interface =
        get_browser_interface(PPB_OPENGLES2_FRAMEBUFFERBLIT_DEV_INTERFACE);
  }
  if (!g_gles2_framebuffer_multisample_interface) {
    g_gles2_framebuffer_multisample_interface =
        get_browser_interface(
            PPB_OPENGLES2_FRAMEBUFFERMULTISAMPLE_DEV_INTERFACE);
  }
  if (!g_gles2_chromium_enable_feature_interface) {
    g_gles2_chromium_enable_feature_interface =
        get_browser_interface(
            PPB_OPENGLES2_CHROMIUMENABLEFEATURE_DEV_INTERFACE);
  }
  if (!g_gles2_chromium_map_sub_interface) {
    g_gles2_chromium_map_sub_interface =
        get_browser_interface(PPB_OPENGLES2_CHROMIUMMAPSUB_DEV_INTERFACE);
  }
  if (!g_gles2_query_interface) {
    g_gles2_query_interface =
        get_browser_interface(PPB_OPENGLES2_QUERY_DEV_INTERFACE);
  }
  return g_gles2_interface ? GL_TRUE : GL_FALSE;
}

GLboolean GL_APIENTRY glTerminatePPAPI() {
  g_gles2_interface = NULL;
  return GL_TRUE;
}

void GL_APIENTRY glSetCurrentContextPPAPI(PP_Resource context) {
  g_current_context = context;
}

PP_Resource GL_APIENTRY glGetCurrentContextPPAPI() {
  return g_current_context;
}

const struct PPB_OpenGLES2* GL_APIENTRY glGetInterfacePPAPI() {
  return g_gles2_interface;
}

const struct PPB_OpenGLES2InstancedArrays_Dev* GL_APIENTRY
    glGetInstancedArraysInterfacePPAPI() {
  return g_gles2_instanced_arrays_interface;
}

const struct PPB_OpenGLES2FramebufferBlit_Dev* GL_APIENTRY
    glGetFramebufferBlitInterfacePPAPI() {
  return g_gles2_framebuffer_blit_interface;
}

const struct PPB_OpenGLES2FramebufferMultisample_Dev* GL_APIENTRY
    glGetFramebufferMultisampleInterfacePPAPI() {
  return g_gles2_framebuffer_multisample_interface;
}

const struct PPB_OpenGLES2ChromiumEnableFeature_Dev* GL_APIENTRY
    glGetChromiumEnableFeatureInterfacePPAPI() {
  return g_gles2_chromium_enable_feature_interface;
}

const struct PPB_OpenGLES2ChromiumMapSub_Dev* GL_APIENTRY
    glGetChromiumMapSubInterfacePPAPI() {
  return g_gles2_chromium_map_sub_interface;
}

const struct PPB_OpenGLES2Query_Dev* GL_APIENTRY
    glGetQueryInterfacePPAPI() {
  return g_gles2_query_interface;
}