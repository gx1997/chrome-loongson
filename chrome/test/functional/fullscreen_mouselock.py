#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil
import time

import pyauto_functional  # Must be imported before pyauto
import pyauto
import test_utils
from selenium.webdriver.common.keys import Keys
from webdriver_pages import settings


class FullscreenMouselockTest(pyauto.PyUITest):
  """TestCase for Fullscreen and Mouse Lock."""

  def setUp(self):
    pyauto.PyUITest.setUp(self)
    self._driver = self.NewWebDriver()
    # Get the hostname pattern (e.g. http://127.0.0.1:57622).
    self._hostname_pattern = (
        re.sub('/files/$', '', self.GetHttpURLForDataPath('')))

  def Debug(self):
    """Test method for experimentation.

    This method will not run automatically.
    """
    page = settings.ContentSettingsPage.FromNavigation(self._driver)
    import pdb
    pdb.set_trace()

  def ExtraChromeFlags(self):
    """Ensures Chrome is launched with custom flags.

    Returns:
      A list of extra flags to pass to Chrome when it is launched.
    """
    # Extra flag needed by scroll performance tests.
    return super(FullscreenMouselockTest,
                 self).ExtraChromeFlags() + ['--enable-pointer-lock']

  def testFullScreenMouseLockHooks(self):
    """Verify fullscreen and mouse lock automation hooks work."""
    self.NavigateToURL(self.GetHttpURLForDataPath(
        'fullscreen_mouselock', 'fullscreen_mouselock.html'))

    # Starting off we shouldn't be fullscreen
    self.assertFalse(self.IsFullscreenForBrowser())
    self.assertFalse(self.IsFullscreenForTab())

    # Go fullscreen
    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(self.IsFullscreenForTab))

    # Bubble should be up prompting to allow fullscreen
    self.assertTrue(self.IsFullscreenBubbleDisplayed())
    self.assertTrue(self.IsFullscreenBubbleDisplayingButtons())
    self.assertTrue(self.IsFullscreenPermissionRequested())

    # Accept bubble, it should go away.
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.WaitUntil(
        lambda: not self.IsFullscreenBubbleDisplayingButtons()))

    # Try to lock mouse, it won't lock yet but permision will be requested.
    self.assertFalse(self.IsMouseLocked())
    self._driver.find_element_by_id('lockMouse1').click()
    self.assertTrue(self.WaitUntil(self.IsMouseLockPermissionRequested))
    self.assertFalse(self.IsMouseLocked())

    # Deny mouse lock.
    self.DenyCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.WaitUntil(
        lambda: not self.IsFullscreenBubbleDisplayingButtons()))
    self.assertFalse(self.IsMouseLocked())

    # Try mouse lock again, and accept it.
    self._driver.find_element_by_id('lockMouse1').click()
    self.assertTrue(self.WaitUntil(self.IsMouseLockPermissionRequested))
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.WaitUntil(self.IsMouseLocked))

    # The following doesn't work - as sending the key to the input field isn't
    # picked up by the browser. :( Need an alternative way.
    #
    # # Ideally we wouldn't target a specific element, we'd just send keys to
    # # whatever the current keyboard focus was.
    # keys_target = driver.find_element_by_id('sendKeysTarget')
    #
    # # ESC key should exit fullscreen and mouse lock.
    #
    # print "# ESC key should exit fullscreen and mouse lock."
    # keys_target.send_keys(Keys.ESCAPE)
    # self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForBrowser()))
    # self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    # self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()))
    #
    # # Check we can go browser fullscreen
    # print "# Check we can go browser fullscreen"
    # keys_target.send_keys(Keys.F11)
    # self.assertTrue(self.WaitUntil(self.IsFullscreenForBrowser))

  def _LaunchFSAndExpectPrompt(self, button_action='enterFullscreen'):
    """Helper function to launch fullscreen and expect a prompt.

    Fullscreen is initiated and a bubble prompt appears asking to allow or
    cancel from fullscreen mode. The actual fullscreen mode doesn't take place
    until after approving the prompt.

    If the helper is not successful then the test will fail.

    Args:
      button_action: The button id to click to initiate an action. Default is to
          click enterFullscreen.
    """
    self.NavigateToURL(self.GetHttpURLForDataPath(
        'fullscreen_mouselock', 'fullscreen_mouselock.html'))
    # Should not be in fullscreen mode during initial launch.
    self.assertFalse(self.IsFullscreenForBrowser())
    self.assertFalse(self.IsFullscreenForTab())
    # Go into fullscreen mode.
    self._driver.find_element_by_id(button_action).click()
    self.assertTrue(self.WaitUntil(self.IsFullscreenForTab))
    # Bubble should display prompting to allow fullscreen.
    self.assertTrue(self.IsFullscreenPermissionRequested())

  def _InitiateBrowserFullscreen(self):
    """Helper function that initiates browser fullscreen."""
    self.NavigateToURL(self.GetHttpURLForDataPath(
        'fullscreen_mouselock', 'fullscreen_mouselock.html'))
    # Should not be in fullscreen mode during initial launch.
    self.assertFalse(self.IsFullscreenForBrowser())
    self.assertFalse(self.IsFullscreenForTab())
    # Initiate browser fullscreen.
    self.ApplyAccelerator(pyauto.IDC_FULLSCREEN)
    self.assertTrue(self.WaitUntil(self.IsFullscreenForBrowser))
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()))

  def _AcceptFullscreenOrMouseLockRequest(self):
    """Helper function to accept fullscreen or mouse lock request."""
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.WaitUntil(
        lambda: not self.IsFullscreenBubbleDisplayingButtons()))

  def _EnableFullscreenAndMouseLockMode(self):
    """Helper function to enable fullscreen and mouse lock mode."""
    self._LaunchFSAndExpectPrompt(button_action='enterFullscreenAndLockMouse1')
    # Allow fullscreen.
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    # The wait is needed due to crbug.com/123396. Should be able to click the
    # fullscreen and mouselock button and be both accepted in a single action.
    self.assertTrue(self.WaitUntil(self.IsMouseLockPermissionRequested))
    # Allow mouse lock.
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.WaitUntil(self.IsMouseLocked))

  def _EnableMouseLockMode(self, button_action='lockMouse1'):
    """Helper function to enable mouse lock mode.

    For now, to lock the mouse, the browser needs to be in fullscreen mode.

    Args:
      button_action: The button id to click to initiate an action. Default is to
          click lockMouse1.
    """
    self._driver.find_element_by_id(button_action).click()
    self.assertTrue(self.IsMouseLockPermissionRequested())
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.assertTrue(self.IsMouseLocked())

  def testPrefsForFullscreenAllowed(self):
    """Verify prefs when fullscreen is allowed."""
    self._LaunchFSAndExpectPrompt()
    self._AcceptFullscreenOrMouseLockRequest()
    content_settings = (
        self.GetPrefsInfo().Prefs()['profile']['content_settings'])
    self.assertEqual(
        {self._hostname_pattern + ',*': {'fullscreen': 1}},  # Allow hostname.
        content_settings['pattern_pairs'],
        msg='Saved hostname pattern does not match expected pattern.')

  def testPrefsForFullscreenExit(self):
    """Verify prefs is empty when exit fullscreen mode before allowing."""
    self._LaunchFSAndExpectPrompt()
    self._driver.find_element_by_id('exitFullscreen').click()
    # Verify exit from fullscreen mode.
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    content_settings = (
        self.GetPrefsInfo().Prefs()['profile']['content_settings'])
    self.assertEqual(
        {}, content_settings['pattern_pairs'],
        msg='Patterns saved when there should be none.')

  def testPatternsForFSAndML(self):
    """Verify hostname pattern and behavior for allowed mouse cursor lock.

    To lock the mouse, the browser needs to be in fullscreen mode.
    """
    self._EnableFullscreenAndMouseLockMode()
    self._EnableMouseLockMode()
    expected_pattern = (
        {self._hostname_pattern + ',*': {'fullscreen': 1, 'mouselock': 1}})
    content_settings = (
        self.GetPrefsInfo().Prefs()['profile']['content_settings'])
    self.assertEqual(
        expected_pattern, content_settings['pattern_pairs'],
        msg='Saved hostname and behavior patterns do not match expected.')

  def testPatternsForAllowMouseLock(self):
    """Verify hostname pattern and behavior for allowed mouse cursor lock.

    Enable fullscreen mode and enable mouse lock separately.
    """
    self._LaunchFSAndExpectPrompt()
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self._EnableMouseLockMode()
    expected_pattern = (
        {self._hostname_pattern + ',*': {'fullscreen': 1, 'mouselock': 1}})
    content_settings = (
        self.GetPrefsInfo().Prefs()['profile']['content_settings'])
    self.assertEqual(
        expected_pattern, content_settings['pattern_pairs'],
        msg='Saved hostname and behavior patterns do not match expected.')

  def testNoMouseLockRequest(self):
    """Verify mouse lock request does not appear.

    When allowing all sites to disable the mouse cursor, the mouse lock request
    bubble should not show. The mouse cursor should be automatically disabled
    when clicking on a disable mouse button.
    """
    # Allow all sites to disable mouse cursor.
    self.SetPrefs(pyauto.kDefaultContentSettings, {u'mouselock': 1})
    self._LaunchFSAndExpectPrompt()
    # Allow for fullscreen mode.
    self._AcceptFullscreenOrMouseLockRequest()
    self._driver.set_script_timeout(2)
    # Receive callback status (success or failure) from javascript that the
    # click has registered and the mouse lock status has changed.
    lock_result = self._driver.execute_async_script(
        'lockMouse1(arguments[arguments.length - 1])')
    self.assertEqual(lock_result, 'success', msg='Mouse lock unsuccessful.')
    self.assertTrue(self.WaitUntil(
        lambda: not self.IsMouseLockPermissionRequested()))
    self.assertTrue(self.IsMouseLocked())

  def testUnableToLockMouse(self):
    """Verify mouse lock is disabled.

    When not allowing any site to disable the mouse cursor, the mouse lock
    request bubble should not show and the mouse cursor should not be disabled.
    """
    # Do not allow any site to disable mouse cursor.
    self.SetPrefs(pyauto.kDefaultContentSettings, {u'mouselock': 2})
    self._LaunchFSAndExpectPrompt()
    # Allow for fullscreen mode.
    self._AcceptFullscreenOrMouseLockRequest()
    self._driver.set_script_timeout(2)
    # Receive callback status (success or failure) from javascript that the
    # click has registered and the mouse lock status has changed.
    lock_result = self._driver.execute_async_script(
        'lockMouse1(arguments[arguments.length - 1])')
    self.assertEqual(lock_result, 'failure', msg='Mouse locked unexpectedly.')
    self.assertTrue(self.WaitUntil(
        lambda: not self.IsMouseLockPermissionRequested()))
    self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()))

  def testEnterTabFSWhileInBrowserFS(self):
    """Verify able to enter into tab fullscreen while in browser fullscreen."""
    self._InitiateBrowserFullscreen()
    # Initiate tab fullscreen.
    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()))

  def testNoMouseLockInBrowserFS(self):
    """Verify mouse lock can't be activated in browser fullscreen.

    Later on when windowed-mode mouse lock is allowed, this test will adjust to
    verify that mouse lock in browser fullscreen requires an allow prompt, even
    when there is a content setting for Allow.
    """
    self._InitiateBrowserFullscreen()
    self._driver.find_element_by_id('lockMouse1').click()
    self._driver.set_script_timeout(2)
    # Receive callback status (success or failure) from javascript that the
    # click has registered and the mouse lock status had changed.
    lock_result = self._driver.execute_async_script(
        'lockMouse1(arguments[arguments.length - 1]);')
    self.assertEqual(
        lock_result, 'failure', msg='Mouse locked in browser fullscreen.')
    self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()),
                    msg='Mouse is locked in browser fullscreen.')

  def testMouseLockExitWhenBrowserLoseFocus(self):
    """Verify mouse lock breaks when browser loses focus.

    Mouse lock breaks when the focus is placed on another new window.
    """
    self.NavigateToURL(self.GetHttpURLForDataPath(
        'fullscreen_mouselock', 'fullscreen_mouselock.html'))
    self._driver.find_element_by_id('enterFullscreen').click()
    self._driver.find_element_by_id('lockMouse1').click()
    self.AcceptCurrentFullscreenOrMouseLockRequest()
    self.WaitUntil(lambda: self.IsFullscreenForTab())
    self.WaitUntil(lambda: self.IsMouseLocked())
    # Open a new window to shift focus away.
    self.OpenNewBrowserWindow(True)
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: not self.IsMouseLocked()),
                    msg='Alert dialog did not break mouse lock.')

  def ExitTabFSToBrowserFS(self):
    """Verify exiting tab fullscreen leaves browser in browser fullscreen.

    The browser initiates browser fullscreen, then initiates tab fullscreen. The
    test verifies that existing tab fullscreen by simulating ESC key press or
    clicking the js function to exitFullscreen() will exit the tab fullscreen
    leaving browser fullscreen intact.
    """
    self._InitiateBrowserFullscreen()
    # Initiate tab fullscreen.
    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForTab()))
    # Require manual intervention to send ESC key due to crbug.com/123930.
    # TODO(dyu): Update to a full test once associated bug is fixed.
    print "Press ESC key to exit tab fullscreen."
    time.sleep(5)
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForBrowser()),
                    msg='Not in browser fullscreen mode.')

    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForTab()))
    # Exit tab fullscreen by clicking button exitFullscreen().
    self._driver.find_element_by_id('exitFullscreen').click()
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForBrowser()),
                    msg='Not in browser fullscreen mode.')

  def F11KeyExitsTabAndBrowserFS(self):
    """Verify existing tab fullscreen exits all fullscreen modes.

    The browser initiates browser fullscreen, then initiates tab fullscreen. The
    test verifies that existing tab fullscreen by simulating F11 key press or
    CMD + SHIFT + F keys on the Mac will exit the tab fullscreen and the
    browser fullscreen.
    """
    self._InitiateBrowserFullscreen()
    # Initiate tab fullscreen.
    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(lambda: self.IsFullscreenForTab()))
    # Require manual intervention to send F11 key due to crbug.com/123930.
    # TODO(dyu): Update to a full test once associated bug is fixed.
    print "Press F11 key to exit tab fullscreen."
    time.sleep(5)
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForTab()))
    self.assertTrue(self.WaitUntil(lambda: not self.IsFullscreenForBrowser()),
                    msg='Browser is in fullscreen mode.')

  def SearchForTextOutsideOfContainer(self):
    """Verify text outside of container is not visible when fullscreen.

    Verify this test manually until there is a way to find text on screen
    without using FindInPage().

    The text that is outside of the fullscreen container should only be visible
    when fullscreen is off. The text should not be visible while in fullscreen
    mode.
    """
    self.NavigateToURL(self.GetHttpURLForDataPath(
        'fullscreen_mouselock', 'fullscreen_mouselock.html'))
    # Should not be in fullscreen mode during initial launch.
    self.assertFalse(self.IsFullscreenForBrowser())
    self.assertFalse(self.IsFullscreenForTab())
    self.assertTrue(
        self.WaitUntil(lambda: self.FindInPage(
            'This text is outside of the container')['match_count'],
                       expect_retval=1))
    # Go into fullscreen mode.
    self._driver.find_element_by_id('enterFullscreen').click()
    self.assertTrue(self.WaitUntil(self.IsFullscreenForTab))
    # TODO(dyu): find a way to verify on screen text instead of using
    #            FindInPage() which searches for text in the HTML.


if __name__ == '__main__':
  pyauto_functional.Main()
