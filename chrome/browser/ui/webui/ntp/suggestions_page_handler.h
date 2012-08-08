// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_SUGGESTIONS_PAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_SUGGESTIONS_PAGE_HANDLER_H_
#pragma once

#include <string>
#include <vector>

#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/history/history_types.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class GURL;
class PageUsageData;
class PrefService;

namespace base {
class ListValue;
class Value;
}

// The handler for Javascript messages related to the "suggestions" view.
//
// This class manages one preference:
// - The URL blacklist: URLs we do not want to show in the thumbnails list.  It
//   is a dictionary for quick access (it associates a dummy boolean to the URL
//   string).
class SuggestionsHandler : public content::WebUIMessageHandler,
                           public content::NotificationObserver {
 public:
  SuggestionsHandler();
  virtual ~SuggestionsHandler();

  // WebUIMessageHandler override and implementation.
  virtual void RegisterMessages() OVERRIDE;

  // Callback for the "getSuggestions" message.
  void HandleGetSuggestions(const base::ListValue* args);

  // Callback for the "blacklistURLFromSuggestions" message.
  void HandleBlacklistURL(const base::ListValue* args);

  // Callback for the "removeURLsFromSuggestionsBlacklist" message.
  void HandleRemoveURLsFromBlacklist(const base::ListValue* args);

  // Callback for the "clearSuggestionsURLsBlacklist" message.
  void HandleClearBlacklist(const base::ListValue* args);

  // Callback for the "suggestedSitesAction" message.
  void HandleSuggestedSitesAction(const base::ListValue* args);

  // Callback for the "suggestedSitesSelected" message.
  void HandleSuggestedSitesSelected(const base::ListValue* args);

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  static void RegisterUserPrefs(PrefService* prefs);

 private:
  // Send a request to the HistoryService to get the suggestions pages.
  void StartQueryForSuggestions();

  // Sets pages_value_ from a format produced by TopSites.
  void SetPagesValueFromTopSites(const history::FilteredURLList& data);

  // Callback for History.
  void OnSuggestionsURLsAvailable(
      CancelableRequestProvider::Handle handle,
      const history::FilteredURLList& data);

  // Puts the passed URL in the blacklist (so it does not show as a thumbnail).
  void BlacklistURL(const GURL& url);

  // Returns the key used in url_blacklist_ for the passed |url|.
  std::string GetDictionaryKeyForURL(const std::string& url);

  // Sends pages_value_ to the javascript side to and resets page_value_.
  void SendPagesValue();

  content::NotificationRegistrar registrar_;

  // Our consumer for the history service page data.
  CancelableRequestConsumerTSimple<PageUsageData*> cancelable_consumer_;

  // Consumer for history service general request(s).
  CancelableRequestConsumer history_consumer_;

  // We pre-fetch the first set of result pages.  This variable is false until
  // we get the first getSuggestions() call.
  bool got_first_suggestions_request_;

  // Keep the results of the db query here.
  scoped_ptr<base::ListValue> pages_value_;

  // Whether the user has viewed the 'suggested' pane.
  bool suggestions_viewed_;

  // Whether the user has performed a "tracked" action to leave the page or not.
  bool user_action_logged_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_SUGGESTIONS_PAGE_HANDLER_H_
