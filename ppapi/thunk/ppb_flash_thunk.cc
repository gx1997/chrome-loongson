// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

void SetInstanceAlwaysOnTop(PP_Instance instance, PP_Bool on_top) {
  EnterInstance enter(instance);
  if (enter.failed())
    return;
  enter.functions()->GetFlashAPI()->SetInstanceAlwaysOnTop(instance, on_top);
}

PP_Bool DrawGlyphs(PP_Instance instance,
                   PP_Resource pp_image_data,
                   const PP_FontDescription_Dev* font_desc,
                   uint32_t color,
                   const PP_Point* position,
                   const PP_Rect* clip,
                   const float transformation[3][3],
                   PP_Bool allow_subpixel_aa,
                   uint32_t glyph_count,
                   const uint16_t glyph_indices[],
                   const PP_Point glyph_advances[]) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->GetFlashAPI()->DrawGlyphs(
      instance, pp_image_data, font_desc, color, position, clip, transformation,
      allow_subpixel_aa, glyph_count, glyph_indices, glyph_advances);
}

PP_Bool DrawGlyphs11(PP_Instance instance,
                     PP_Resource pp_image_data,
                     const PP_FontDescription_Dev* font_desc,
                     uint32_t color,
                     PP_Point position,
                     PP_Rect clip,
                     const float transformation[3][3],
                     uint32_t glyph_count,
                     const uint16_t glyph_indices[],
                     const PP_Point glyph_advances[]) {
  // Backwards-compatible version. DrawGlyphs locks; no need to lock here.
  return DrawGlyphs(instance, pp_image_data, font_desc, color, &position,
                    &clip, transformation, PP_TRUE, glyph_count, glyph_indices,
                    glyph_advances);
}

PP_Var GetProxyForURL(PP_Instance instance, const char* url) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetFlashAPI()->GetProxyForURL(instance, url);
}

int32_t Navigate(PP_Resource request_id,
                 const char* target,
                 PP_Bool from_user_action) {
  // TODO(brettw): this function should take an instance.
  // To work around this, use the PP_Instance from the resource.
  PP_Instance instance;
  {
    thunk::EnterResource<thunk::PPB_URLRequestInfo_API> enter(request_id, true);
    if (enter.failed())
      return PP_ERROR_BADRESOURCE;
    instance = enter.resource()->pp_instance();
  }

  EnterInstance enter(instance);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->GetFlashAPI()->Navigate(instance, request_id,
                                                    target, from_user_action);
}

int32_t Navigate11(PP_Resource request_id,
                   const char* target,
                   bool from_user_action) {
  // Backwards-compatible version. Navigate locks; no need to lock here.
  return Navigate(request_id, target, PP_FromBool(from_user_action));
}

void RunMessageLoop(PP_Instance instance) {
  EnterInstance enter(instance);
  if (enter.failed())
    return;
  enter.functions()->GetFlashAPI()->RunMessageLoop(instance);
}

void QuitMessageLoop(PP_Instance instance) {
  EnterInstance enter(instance);
  if (enter.failed())
    return;
  enter.functions()->GetFlashAPI()->QuitMessageLoop(instance);
}

double GetLocalTimeZoneOffset(PP_Instance instance, PP_Time t) {
  EnterInstance enter(instance);
  if (enter.failed())
    return 0.0;
  return enter.functions()->GetFlashAPI()->GetLocalTimeZoneOffset(instance, t);
}

PP_Var GetCommandLineArgs(PP_Module /* pp_module */) {
  // There's no instance so we have to reach into the globals without thunking.
  ProxyAutoLock lock;
  return StringVar::StringToPPVar(PpapiGlobals::Get()->GetCmdLine());
}

void PreLoadFontWin(const void* logfontw) {
  // There's no instance so we have to reach into the delegate without
  // thunking.
  ProxyAutoLock lock;
  PpapiGlobals::Get()->PreCacheFontForFlash(logfontw);
}

PP_Bool IsRectTopmost(PP_Instance instance, const PP_Rect* rect) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->GetFlashAPI()->IsRectTopmost(instance, rect);
}

int32_t InvokePrinting(PP_Instance instance) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->GetFlashAPI()->InvokePrinting(instance);
}

void UpdateActivity(PP_Instance instance) {
  EnterInstance enter(instance);
  if (enter.failed())
    return;
  enter.functions()->GetFlashAPI()->UpdateActivity(instance);
}

PP_Var GetDeviceID(PP_Instance instance) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.functions()->GetFlashAPI()->GetDeviceID(instance);
}

const PPB_Flash_11 g_ppb_flash_11_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs11,
  &GetProxyForURL,
  &Navigate11,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs
};

const PPB_Flash_12_0 g_ppb_flash_12_0_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin
};

const PPB_Flash_12_1 g_ppb_flash_12_1_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &InvokePrinting,
  &UpdateActivity
};

const PPB_Flash_12_2 g_ppb_flash_12_2_thunk = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &Navigate,
  &RunMessageLoop,
  &QuitMessageLoop,
  &GetLocalTimeZoneOffset,
  &GetCommandLineArgs,
  &PreLoadFontWin,
  &IsRectTopmost,
  &InvokePrinting,
  &UpdateActivity,
  &GetDeviceID
};

}  // namespace

const PPB_Flash_11* GetPPB_Flash_11_Thunk() {
  return &g_ppb_flash_11_thunk;
}

const PPB_Flash_12_0* GetPPB_Flash_12_0_Thunk() {
  return &g_ppb_flash_12_0_thunk;
}

const PPB_Flash_12_1* GetPPB_Flash_12_1_Thunk() {
  return &g_ppb_flash_12_1_thunk;
}

const PPB_Flash_12_2* GetPPB_Flash_12_2_Thunk() {
  return &g_ppb_flash_12_2_thunk;
}

}  // namespace thunk
}  // namespace ppapi
