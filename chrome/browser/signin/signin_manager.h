// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The signin manager encapsulates some functionality tracking
// which user is signed in. When a user is signed in, a ClientLogin
// request is run on their behalf. Auth tokens are fetched from Google
// and the results are stored in the TokenService.
//
// **NOTE** on semantics of SigninManager:
//
// Once a signin is successful, the username becomes "established" and will not
// be cleared until a SignOut operation is performed (persists across
// restarts). Until that happens, the signin manager can still be used to
// refresh credentials, but changing the username is not permitted.

#ifndef CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_H_
#define CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_H_
#pragma once

#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/profiles/profile_keyed_service.h"
#include "chrome/common/net/gaia/gaia_auth_consumer.h"
#include "chrome/common/net/gaia/google_service_auth_error.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class GaiaAuthFetcher;
class Profile;
class PrefService;

// Details for the Notification type GOOGLE_SIGNIN_SUCCESSFUL.
// A listener might use this to make note of a username / password
// pair for encryption keys.
struct GoogleServiceSigninSuccessDetails {
  GoogleServiceSigninSuccessDetails(const std::string& in_username,
                                    const std::string& in_password)
      : username(in_username),
        password(in_password) {}
  std::string username;
  std::string password;
};

class SigninManager : public GaiaAuthConsumer,
                      public content::NotificationObserver,
                      public ProfileKeyedService {
 public:
  // Returns true if the cookie policy for the given profile allows cookies
  // for the Google signin domain.
  static bool AreSigninCookiesAllowed(Profile* profile);

  SigninManager();
  virtual ~SigninManager();

  // If user was signed in, load tokens from DB if available.
  void Initialize(Profile* profile);
  bool IsInitialized() const;

  // If a user has previously established a username and SignOut has not been
  // called, this will return the username.
  // Otherwise, it will return an empty string.
  const std::string& GetAuthenticatedUsername();

  // Sets the user name.  Note: |username| should be already authenticated as
  // this is a sticky operation (in contrast to StartSignIn).
  // TODO(tim): Remove this in favor of passing username on construction by
  // (by platform / depending on StartBehavior). Bug 88109.
  void SetAuthenticatedUsername(const std::string& username);

  // Attempt to sign in this user with ClientLogin. If successful, set a
  // preference indicating the signed in user and send out a notification,
  // then start fetching tokens for the user.
  // This is overridden for test subclasses that don't want to issue auth
  // requests.
  virtual void StartSignIn(const std::string& username,
                           const std::string& password,
                           const std::string& login_token,
                           const std::string& login_captcha);

  // Used when a second factor access code was required to complete a signin
  // attempt.
  void ProvideSecondFactorAccessCode(const std::string& access_code);

  // Attempt to sign in this user with existing credentials from the cookie jar.
  // |session_index| indicates which user account to use if the cookie jar
  // contains a multi-login session. Otherwise the end result of this call is
  // the same as StartSignIn().
  virtual void StartSignInWithCredentials(const std::string& session_index,
                                          const std::string& username,
                                          const std::string& password);

  // Sign a user out, removing the preference, erasing all keys
  // associated with the user, and canceling all auth in progress.
  virtual void SignOut();

  // Returns true if there's a signin in progress. Virtual so it can be
  // overridden by mocks.
  virtual bool AuthInProgress() const;

  // Handles errors if a required user info key is not returned from the
  // GetUserInfo call.
  void OnGetUserInfoKeyNotFound(const std::string& key);

  // GaiaAuthConsumer
  virtual void OnClientLoginSuccess(const ClientLoginResult& result) OVERRIDE;
  virtual void OnClientLoginFailure(
      const GoogleServiceAuthError& error) OVERRIDE;
  virtual void OnClientOAuthSuccess(const ClientOAuthResult& result) OVERRIDE;
  virtual void OnClientOAuthFailure(
      const GoogleServiceAuthError& error) OVERRIDE;
  virtual void OnGetUserInfoSuccess(const UserInfoMap& data) OVERRIDE;
  virtual void OnGetUserInfoFailure(
      const GoogleServiceAuthError& error) OVERRIDE;
  virtual void OnTokenAuthSuccess(const net::ResponseCookies& cookies,
                                  const std::string& data) OVERRIDE;
  virtual void OnTokenAuthFailure(const GoogleServiceAuthError& error) OVERRIDE;
  virtual void OnUberAuthTokenSuccess(const std::string& token) OVERRIDE;
  virtual void OnUberAuthTokenFailure(
      const GoogleServiceAuthError& error) OVERRIDE;

  // content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

 private:
  friend class FakeSigninManager;
  FRIEND_TEST_ALL_PREFIXES(SigninManagerTest, ClearTransientSigninData);
  FRIEND_TEST_ALL_PREFIXES(SigninManagerTest, ProvideSecondFactorSuccess);
  FRIEND_TEST_ALL_PREFIXES(SigninManagerTest, ProvideSecondFactorFailure);
  void PrepareForSignin();

  // Called when a new request to re-authenticate a user is in progress.
  // Will clear in memory data but leaves the db as such so when the browser
  // restarts we can use the old token(which might throw a password error).
  void ClearTransientSigninData();

  // Called to handle an error from a GAIA auth fetch.  Sets the last error
  // to |error|, sends out a notification of login failure, and clears the
  // transient signin data.
  void HandleAuthError(const GoogleServiceAuthError& error);

  Profile* profile_;

  // ClientLogin identity.
  std::string possibly_invalid_username_;
  std::string password_;  // This is kept empty whenever possible.
  bool had_two_factor_error_;

  void CleanupNotificationRegistration();

  // Result of the last client login, kept pending the lookup of the
  // canonical email.
  ClientLoginResult last_result_;

  // Actual client login handler.
  scoped_ptr<GaiaAuthFetcher> client_login_;

  // Register for notifications from the TokenService.
  content::NotificationRegistrar registrar_;

  std::string authenticated_username_;

  DISALLOW_COPY_AND_ASSIGN(SigninManager);
};

#endif  // CHROME_BROWSER_SIGNIN_SIGNIN_MANAGER_H_
