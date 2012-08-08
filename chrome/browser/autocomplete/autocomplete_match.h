// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_MATCH_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_MATCH_H_
#pragma once

#include <vector>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "content/public/common/page_transition_types.h"
#include "googleurl/src/gurl.h"

class AutocompleteProvider;
class TemplateURL;

// AutocompleteMatch ----------------------------------------------------------

// A single result line with classified spans.  The autocomplete popup displays
// the 'contents' and the 'description' (the description is optional) in the
// autocomplete dropdown, and fills in 'fill_into_edit' into the textbox when
// that line is selected.  fill_into_edit may be the same as 'description' for
// things like URLs, but may be different for searches or other providers.  For
// example, a search result may say "Search for asdf" as the description, but
// "asdf" should appear in the box.
struct AutocompleteMatch {
  // Autocomplete matches contain strings that are classified according to a
  // separate vector of styles.  This vector associates flags with particular
  // string segments, and must be in sorted order.  All text must be associated
  // with some kind of classification.  Even if a match has no distinct
  // segments, its vector should contain an entry at offset 0 with no flags.
  //
  // Example: The user typed "goog"
  //   http://www.google.com/        Google
  //   ^          ^   ^              ^   ^
  //   0,         |   15,            |   4,
  //              11,match           0,match
  //
  // This structure holds the classification information for each span.
  struct ACMatchClassification {
    // The values in here are not mutually exclusive -- use them like a
    // bitfield.  This also means we use "int" instead of this enum type when
    // passing the values around, so the compiler doesn't complain.
    enum Style {
      NONE  = 0,
      URL   = 1 << 0,  // A URL
      MATCH = 1 << 1,  // A match for the user's search term
      DIM   = 1 << 2,  // "Helper text"
    };

    ACMatchClassification(size_t offset, int style)
        : offset(offset),
          style(style) {
    }

    // Offset within the string that this classification starts
    size_t offset;

    int style;
  };

  typedef std::vector<ACMatchClassification> ACMatchClassifications;

  // The type of this match.
  enum Type {
    URL_WHAT_YOU_TYPED = 0,  // The input as a URL.
    HISTORY_URL,             // A past page whose URL contains the input.
    HISTORY_TITLE,           // A past page whose title contains the input.
    HISTORY_BODY,            // A past page whose body contains the input.
    HISTORY_KEYWORD,         // A past page whose keyword contains the input.
    NAVSUGGEST,              // A suggested URL.
    SEARCH_WHAT_YOU_TYPED,   // The input as a search query (with the default
                             // engine).
    SEARCH_HISTORY,          // A past search (with the default engine)
                             // containing the input.
    SEARCH_SUGGEST,          // A suggested search (with the default engine).
    SEARCH_OTHER_ENGINE,     // A search with a non-default engine.
    EXTENSION_APP,           // An Extension App with a title/url that contains
                             // the input.
    NUM_TYPES,
  };

  // Null-terminated array of characters that are not valid within |contents|
  // and |description| strings.
  static const char16 kInvalidChars[];

  AutocompleteMatch();
  AutocompleteMatch(AutocompleteProvider* provider,
                    int relevance,
                    bool deletable,
                    Type type);
  AutocompleteMatch(const AutocompleteMatch& match);
  ~AutocompleteMatch();

  // Converts |type| to a string representation.  Used in logging and debugging.
  AutocompleteMatch& operator=(const AutocompleteMatch& match);

  // Converts |type| to a string representation.  Used in logging.
  static std::string TypeToString(Type type);

  // Converts |type| to a resource identifier for the appropriate icon for this
  // type.
  static int TypeToIcon(Type type);

  // Comparison function for determining when one match is better than another.
  static bool MoreRelevant(const AutocompleteMatch& elem1,
                           const AutocompleteMatch& elem2);

  // Comparison functions for removing matches with duplicate destinations.
  // Destinations are compared using |stripped_destination_url|.
  static bool DestinationSortFunc(const AutocompleteMatch& elem1,
                                  const AutocompleteMatch& elem2);
  static bool DestinationsEqual(const AutocompleteMatch& elem1,
                                const AutocompleteMatch& elem2);

  // Helper functions for classes creating matches:
  // Fills in the classifications for |text|, using |style| as the base style
  // and marking the first instance of |find_text| as a match.  (This match
  // will also not be dimmed, if |style| has DIM set.)
  static void ClassifyMatchInString(const string16& find_text,
                                    const string16& text,
                                    int style,
                                    ACMatchClassifications* classifications);

  // Similar to ClassifyMatchInString(), but for cases where the range to mark
  // as matching is already known (avoids calling find()).  This can be helpful
  // when find() would be misleading (e.g. you want to mark the second match in
  // a string instead of the first).
  static void ClassifyLocationInString(size_t match_location,
                                       size_t match_length,
                                       size_t overall_length,
                                       int style,
                                       ACMatchClassifications* classifications);

  // Converts classifications to and from a serialized string representation
  // (using comma-separated integers to sequentially list positions and styles).
  static std::string ClassificationsToString(
      const ACMatchClassifications& classifications);
  static ACMatchClassifications ClassificationsFromString(
      const std::string& serialized_classifications);

  // Adds a classification to the end of |classifications| iff its style is
  // different from the last existing classification.  |offset| must be larger
  // than the offset of the last classification in |classifications|.
  static void AddLastClassificationIfNecessary(
      ACMatchClassifications* classifications,
      size_t offset,
      int style);

  // Removes invalid characters from |text|. Should be called on strings coming
  // from external sources (such as extensions) before assigning to |contents|
  // or |description|.
  static string16 SanitizeString(const string16& text);

  // Copies the destination_url with "www." stripped off to
  // |stripped_destination_url|.  This method is invoked internally by the
  // AutocompleteController and does not normally need to be invoked.
  void ComputeStrippedDestinationURL();

  // Gets data relevant to whether there should be any special keyword-related
  // UI shown for this match.  If this match represents a selected keyword, i.e.
  // the UI should be "in keyword mode", |keyword| will be set to the keyword
  // and |is_keyword_hint| will be set to false.  If this match has a non-NULL
  // |associated_keyword|, i.e. we should show a "Press [tab] to search ___"
  // hint and allow the user to toggle into keyword mode, |keyword| will be set
  // to the associated keyword and |is_keyword_hint| will be set to true.  Note
  // that only one of these states can be in effect at once.  In all other
  // cases, |keyword| will be cleared, even when our member variable |keyword|
  // is non-empty.  See also GetSubstitutingExplicitlyInvokedKeyword().
  void GetKeywordUIState(string16* keyword,
                         bool* is_keyword_hint) const;

  // Returns |keyword|, but only if it represents a substituting keyword that
  // the user has explicitly invoked.  If for example this match represents a
  // search with the default search engine (and the user didn't explicitly
  // invoke its keyword), this returns the empty string.  The result is that
  // this function returns a non-empty string in the same cases as when the UI
  // should show up as being "in keyword mode".
  string16 GetSubstitutingExplicitlyInvokedKeyword() const;

  // Returns the TemplateURL associated with this match.
  TemplateURL* GetTemplateURL() const;

  // The provider of this match, used to remember which provider the user had
  // selected when the input changes. This may be NULL, in which case there is
  // no provider (or memory of the user's selection).
  AutocompleteProvider* provider;

  // The relevance of this match. See table in autocomplete.h for scores
  // returned by various providers. This is used to rank matches among all
  // responding providers, so different providers must be carefully tuned to
  // supply matches with appropriate relevance.
  //
  // TODO(pkasting): http://b/1111299 This should be calculated algorithmically,
  // rather than being a fairly fixed value defined by the table above.
  int relevance;

  // True if the user should be able to delete this match.
  bool deletable;

  // This string is loaded into the location bar when the item is selected
  // by pressing the arrow keys. This may be different than a URL, for example,
  // for search suggestions, this would just be the search terms.
  string16 fill_into_edit;

  // The position within fill_into_edit from which we'll display the inline
  // autocomplete string.  This will be string16::npos if this match should
  // not be inline autocompleted.
  size_t inline_autocomplete_offset;

  // The URL to actually load when the autocomplete item is selected. This URL
  // should be canonical so we can compare URLs with strcmp to avoid dupes.
  // It may be empty if there is no possible navigation.
  GURL destination_url;

  // The destination URL with "www." stripped off for better dupe finding.
  GURL stripped_destination_url;

  // The main text displayed in the address bar dropdown.
  string16 contents;
  ACMatchClassifications contents_class;

  // Additional helper text for each entry, such as a title or description.
  string16 description;
  ACMatchClassifications description_class;

  // The transition type to use when the user opens this match.  By default
  // this is TYPED.  Providers whose matches do not look like URLs should set
  // it to GENERATED.
  content::PageTransition transition;

  // True when this match is the "what you typed" match from the history
  // system.
  bool is_history_what_you_typed_match;

  // Type of this match.
  Type type;

  // Set with a keyword provider match if this match can show a keyword hint.
  // For example, if this is a SearchProvider match for "www.amazon.com",
  // |associated_keyword| could be a KeywordProvider match for "amazon.com".
  scoped_ptr<AutocompleteMatch> associated_keyword;

  // For matches that correspond to valid substituting keywords ("search
  // engines" that aren't the default engine, or extension keywords), this
  // is the keyword.  If this is set, then when displaying this match, the
  // edit will use the "keyword mode" UI that shows a blue
  // "Search <engine name>" chit before the user's typing.  This should be
  // set for any match that's an |associated_keyword| of a match in the main
  // result list, as well as any other matches in the main result list that
  // are direct keyword matches (e.g. if the user types in a keyword name and
  // some search terms directly).
  string16 keyword;

  // Indicates the TemplateURL the match originated from. This is set for
  // keywords as well as matches for the default search provider.
  TemplateURL* template_url;

  // True if the user has starred the destination URL.
  bool starred;

  // True if this match is from a previous result.
  bool from_previous;

#ifndef NDEBUG
  // Does a data integrity check on this match.
  void Validate() const;

  // Checks one text/classifications pair for valid values.
  void ValidateClassifications(
      const string16& text,
      const ACMatchClassifications& classifications) const;
#endif
};

typedef AutocompleteMatch::ACMatchClassification ACMatchClassification;
typedef std::vector<ACMatchClassification> ACMatchClassifications;

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_MATCH_H_
