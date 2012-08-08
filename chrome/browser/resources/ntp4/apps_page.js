// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('ntp', function() {
  'use strict';

  var APP_LAUNCH = {
    // The histogram buckets (keep in sync with extension_constants.h).
    NTP_APPS_MAXIMIZED: 0,
    NTP_APPS_COLLAPSED: 1,
    NTP_APPS_MENU: 2,
    NTP_MOST_VISITED: 3,
    NTP_RECENTLY_CLOSED: 4,
    NTP_APP_RE_ENABLE: 16,
    NTP_WEBSTORE_FOOTER: 18,
    NTP_WEBSTORE_PLUS_ICON: 19,
  };

  // Histogram buckets for UMA tracking of where a DnD drop came from.
  var DRAG_SOURCE = {
    SAME_APPS_PANE: 0,
    OTHER_APPS_PANE: 1,
    MOST_VISITED_PANE: 2,
    BOOKMARKS_PANE: 3,
    OUTSIDE_NTP: 4
  };
  var DRAG_SOURCE_LIMIT = DRAG_SOURCE.OUTSIDE_NTP + 1;

  /** @const */ var appInstallHintTileLimit = 10;

  /**
   * App context menu. The class is designed to be used as a singleton with
   * the app that is currently showing a context menu stored in this.app_.
   * @constructor
   */
  function AppContextMenu() {
    this.__proto__ = AppContextMenu.prototype;
    this.initialize();
  }
  cr.addSingletonGetter(AppContextMenu);

  AppContextMenu.prototype = {
    initialize: function() {
      var menu = new cr.ui.Menu;
      cr.ui.decorate(menu, cr.ui.Menu);
      menu.classList.add('app-context-menu');
      this.menu = menu;

      this.launch_ = this.appendMenuItem_();
      this.launch_.addEventListener('activate', this.onLaunch_.bind(this));

      menu.appendChild(cr.ui.MenuItem.createSeparator());
      this.launchRegularTab_ = this.appendMenuItem_('applaunchtyperegular');
      this.launchPinnedTab_ = this.appendMenuItem_('applaunchtypepinned');
      if (!cr.isMac)
        this.launchNewWindow_ = this.appendMenuItem_('applaunchtypewindow');
      this.launchFullscreen_ = this.appendMenuItem_('applaunchtypefullscreen');

      var self = this;
      this.forAllLaunchTypes_(function(launchTypeButton, id) {
        launchTypeButton.addEventListener('activate',
            self.onLaunchTypeChanged_.bind(self));
      });

      menu.appendChild(cr.ui.MenuItem.createSeparator());
      this.options_ = this.appendMenuItem_('appoptions');
      this.disableNotifications_ =
          this.appendMenuItem_('appdisablenotifications');
      this.uninstall_ = this.appendMenuItem_('appuninstall');
      this.options_.addEventListener('activate',
                                     this.onShowOptions_.bind(this));
      this.disableNotifications_.addEventListener(
          'activate', this.onDisableNotifications_.bind(this));
      this.uninstall_.addEventListener('activate',
                                       this.onUninstall_.bind(this));

      if (!cr.isMac && !cr.isChromeOS) {
        menu.appendChild(cr.ui.MenuItem.createSeparator());
        this.createShortcut_ = this.appendMenuItem_('appcreateshortcut');
        this.createShortcut_.addEventListener(
            'activate', this.onCreateShortcut_.bind(this));
      }

      document.body.appendChild(menu);
    },

    /**
     * Appends a menu item to |this.menu|.
     * @param {?String} textId If non-null, the ID for the localized string
     *     that acts as the item's label.
     */
    appendMenuItem_: function(textId) {
      var button = cr.doc.createElement('button');
      this.menu.appendChild(button);
      cr.ui.decorate(button, cr.ui.MenuItem);
      if (textId)
        button.textContent = loadTimeData.getString(textId);
      return button;
    },

    /**
     * Iterates over all the launch type menu items.
     * @param {function(cr.ui.MenuItem, number)} f The function to call for each
     *     menu item. The parameters to the function include the menu item and
     *     the associated launch ID.
     */
    forAllLaunchTypes_: function(f) {
      // Order matters: index matches launchType id.
      var launchTypes = [this.launchPinnedTab_,
                         this.launchRegularTab_,
                         this.launchFullscreen_,
                         this.launchNewWindow_];

      for (var i = 0; i < launchTypes.length; ++i) {
        if (!launchTypes[i])
          continue;

        f(launchTypes[i], i);
      }
    },

    /**
     * Does all the necessary setup to show the menu for the given app.
     * @param {App} app The App object that will be showing a context menu.
     */
    setupForApp: function(app) {
      this.app_ = app;

      this.launch_.textContent = app.appData.title;

      this.forAllLaunchTypes_(function(launchTypeButton, id) {
        launchTypeButton.disabled = false;
        launchTypeButton.checked = app.appData.launch_type == id;
      });

      this.options_.disabled = !app.appData.optionsUrl || !app.appData.enabled;
      this.uninstall_.disabled = !app.appData.mayDisable;

      this.disableNotifications_.hidden = true;
      var notificationsDisabled = app.appData.notifications_disabled;
      if (typeof notificationsDisabled != 'undefined') {
        this.disableNotifications_.hidden = false;
        this.disableNotifications_.checked = notificationsDisabled;
      }
    },

    /**
     * Handlers for menu item activation.
     * @param {Event} e The activation event.
     * @private
     */
    onLaunch_: function(e) {
      chrome.send('launchApp', [this.app_.appId, APP_LAUNCH.NTP_APPS_MENU]);
    },
    onLaunchTypeChanged_: function(e) {
      var pressed = e.currentTarget;
      var app = this.app_;
      this.forAllLaunchTypes_(function(launchTypeButton, id) {
        if (launchTypeButton == pressed) {
          chrome.send('setLaunchType', [app.appId, id]);
          // Manually update the launch type. We will only get
          // appsPrefChangeCallback calls after changes to other NTP instances.
          app.appData.launch_type = id;
        }
      });
    },
    onShowOptions_: function(e) {
      window.location = this.app_.appData.optionsUrl;
    },
    onDisableNotifications_: function(e) {
      var app = this.app_;
      app.removeBubble();
      // Toggle the current disable setting.
      var newSetting = !this.disableNotifications_.checked;
      app.appData.notifications_disabled = newSetting;
      chrome.send('setNotificationsDisabled', [app.appData.id, newSetting]);
    },
    onUninstall_: function(e) {
      chrome.send('uninstallApp', [this.app_.appData.id]);
    },
    onCreateShortcut_: function(e) {
      chrome.send('createAppShortcut', [this.app_.appData.id]);
    },
  };

  /**
   * Creates a new App object.
   * @param {Object} appData The data object that describes the app.
   * @constructor
   * @extends {HTMLDivElement}
   */
  function App(appData) {
    var el = cr.doc.createElement('div');
    el.__proto__ = App.prototype;
    el.initialize(appData);

    return el;
  }

  App.prototype = {
    __proto__: HTMLDivElement.prototype,

    /**
     * Initialize the app object.
     * @param {Object} appData The data object that describes the app.
     */
    initialize: function(appData) {
      this.appData = appData;
      assert(this.appData_.id, 'Got an app without an ID');
      this.id = this.appData_.id;

      this.className = 'app focusable';

      if (!this.appData_.icon_big_exists && this.appData_.icon_small_exists)
        this.useSmallIcon_ = true;

      this.appContents_ = this.useSmallIcon_ ?
          $('app-small-icon-template').cloneNode(true) :
          $('app-large-icon-template').cloneNode(true);
      this.appContents_.id = '';
      this.appendChild(this.appContents_);

      this.appImgContainer_ = this.querySelector('.app-img-container');
      this.appImg_ = this.appImgContainer_.querySelector('img');
      this.setIcon();

      if (this.useSmallIcon_) {
        this.imgDiv_ = this.querySelector('.app-icon-div');
        this.addLaunchClickTarget_(this.imgDiv_);
        this.imgDiv_.title = this.appData_.title;
        chrome.send('getAppIconDominantColor', [this.id]);
      } else {
        this.addLaunchClickTarget_(this.appImgContainer_);
        this.appImgContainer_.title = this.appData_.title;
      }

      var appSpan = this.appContents_.querySelector('.title');
      appSpan.textContent = appSpan.title = this.appData_.title;
      this.addLaunchClickTarget_(appSpan);

      var notification = this.appData_.notification;
      var hasNotification = typeof notification != 'undefined' &&
                            typeof notification['title'] != 'undefined' &&
                            typeof notification['body'] != 'undefined' &&
                            !this.appData_.notifications_disabled;
      if (hasNotification)
        this.setupNotification_(notification);

      this.addEventListener('keydown', cr.ui.contextMenuHandler);
      this.addEventListener('keyup', cr.ui.contextMenuHandler);

      // This hack is here so that appContents.contextMenu will be the same as
      // this.contextMenu.
      var self = this;
      this.appContents_.__defineGetter__('contextMenu', function() {
        return self.contextMenu;
      });
      this.appContents_.addEventListener('contextmenu',
                                         cr.ui.contextMenuHandler);

      if (this.appData_.is_webstore)
        this.createAppsPromoExtras_();

      this.addEventListener('mousedown', this.onMousedown_, true);
      this.addEventListener('keydown', this.onKeydown_);
      this.addEventListener('blur', this.onBlur_, true);
    },

    /**
     * Sets the color of the favicon dominant color bar.
     * @param {string} color The css-parsable value for the color.
     */
    set stripeColor(color) {
      this.querySelector('.color-stripe').style.backgroundColor = color;
    },

    /**
     * Removes the app tile from the page. Should be called after the app has
     * been uninstalled.
     */
    remove: function(opt_animate) {
      // Unset the ID immediately, because the app is already gone. But leave
      // the tile on the page as it animates out.
      this.id = '';
      this.tile.doRemove(opt_animate);
    },

    /**
     * Set the URL of the icon from |appData_|. This won't actually show the
     * icon until loadIcon() is called (for performance reasons; we don't want
     * to load icons until we have to).
     */
    setIcon: function() {
      var src = this.useSmallIcon_ ? this.appData_.icon_small :
                                     this.appData_.icon_big;
      if (!this.appData_.enabled ||
          (!this.appData_.offlineEnabled && !navigator.onLine)) {
        src += '?grayscale=true';
      }

      this.appImgSrc_ = src;
      this.classList.add('icon-loading');
    },

    /**
     * Shows the icon for the app. That is, it causes chrome to load the app
     * icon resource.
     */
    loadIcon: function() {
      if (this.appImgSrc_) {
        this.appImg_.src = this.appImgSrc_;
        this.appImg_.classList.remove('invisible');
        this.appImgSrc_ = null;
      }

      this.classList.remove('icon-loading');
    },

    /**
     * Creates a bubble node.
     * @param {Object} notification The notification to show in the bubble.
     * @param {boolean} full Whether we want the headline or just the content.
     * @private
     */
    createBubbleNode_: function(notification, full) {
      if (!full) {
        var titleItem = this.ownerDocument.createElement('span');
        titleItem.textContent = notification['title'];
        return titleItem;
      } else {
        var container = this.ownerDocument.createElement('div');

        var messageItem = this.ownerDocument.createElement('div');
        messageItem.textContent = notification['body'];
        container.appendChild(messageItem);

        if (notification['linkUrl'] && notification['linkText']) {
          var anchor = this.ownerDocument.createElement('a');
          anchor.href = notification['linkUrl'];
          anchor.textContent = notification['linkText'];
          container.appendChild(anchor);
        }

        return container;
      }
    },

    /**
     * Sets up a notification for the app icon.
     * @param {Object} notification The notification to show in the bubble.
     * @private
     */
    setupNotification_: function(notification) {
      if (notification) {
        var infoBubble;
        if (!this.currentBubbleShowing_) {
          // Create a new bubble.
          infoBubble = new cr.ui.ExpandableBubble;
          infoBubble.anchorNode = this;
          infoBubble.appId = this.appData_.id;
          infoBubble.handleCloseEvent = function() {
            chrome.send('closeNotification', [this.appId]);
            infoBubble.hide();
          };
        } else {
          // Reuse the old bubble instead of popping up a new bubble over
          // the old one.
          infoBubble = this.currentBubbleShowing_;
          infoBubble.collapseBubble_();
        }
        infoBubble.contentTitle = this.createBubbleNode_(notification, false);
        infoBubble.content = this.createBubbleNode_(notification, true);
        infoBubble.show();
        infoBubble.resizeAndReposition();

        this.currentBubbleShowing_ = infoBubble;
      }
    },

    /**
     *  Removes the info bubble if there is one.
     */
    removeBubble: function() {
      if (this.currentBubbleShowing_) {
        this.currentBubbleShowing_.hide();
        this.currentBubbleShowing_ = null;
      }
    },

    /**
     * Creates the apps-promo section of the app (should only be called for the
     * webstore app).
     * @private
     */
    createAppsPromoExtras_: function() {
      this.classList.add('webstore');

      this.appsPromoExtras_ = $('apps-promo-extras-template').cloneNode(true);
      this.appsPromoExtras_.id = '';
      this.appsPromoHeading_ =
          this.appsPromoExtras_.querySelector('.apps-promo-heading');
      this.appsPromoLink_ =
          this.appsPromoExtras_.querySelector('.apps-promo-link');
      this.appsPromoLink_.addEventListener('click', this.onClick_.bind(this));

      this.appsPromoLogo_ = this.ownerDocument.createElement('img');
      this.appsPromoLogo_.className = 'apps-promo-logo';
      this.appImgContainer_.appendChild(this.appsPromoLogo_);

      this.appendChild(this.appsPromoExtras_);
    },

    /**
     * Sets the apps promo appearance. If |data| is null, there is no promo. If
     * |data| is non-null, it contains strings to be shown for the promo. The
     * promo is only shown when the webstore app icon is alone on a page.
     * @param {Object} data A dictionary that contains apps promo strings.
     */
    setAppsPromoData: function(data) {
      if (data) {
        this.classList.add('has-promo');
      } else {
        this.classList.remove('has-promo');
        return;
      }

      this.appsPromoHeading_.textContent = data.promoHeader;
      this.appsPromoLink_.href = data.promoLink;
      this.appsPromoLink_.textContent = data.promoButton;
      this.appsPromoLogo_.src = data.promoLogo;
    },

    /**
     * Set the size and position of the app tile.
     * @param {number} size The total size of |this|.
     * @param {number} x The x-position.
     * @param {number} y The y-position.
     *     animate.
     */
    setBounds: function(size, x, y) {
      var imgSize = size * APP_IMG_SIZE_FRACTION;
      this.appImgContainer_.style.width = this.appImgContainer_.style.height =
          this.useSmallIcon_ ? '16px' : imgSize + 'px';
      if (this.useSmallIcon_) {
        // 3/4 is the ratio of 96px to 128px (the used height and full height
        // of icons in apps).
        var iconSize = imgSize * 3 / 4;
        // The -2 is for the div border to improve the visual alignment for the
        // icon div.
        this.imgDiv_.style.width = this.imgDiv_.style.height =
            (iconSize - 2) + 'px';
        // Margins set to get the icon placement right and the text to line up.
        this.imgDiv_.style.marginTop = this.imgDiv_.style.marginBottom =
            ((imgSize - iconSize) / 2) + 'px';
      }

      this.style.width = this.style.height = size + 'px';
      this.style.left = x + 'px';
      this.style.right = x + 'px';
      this.style.top = y + 'px';

      if (this.currentBubbleShowing_)
        this.currentBubbleShowing_.resizeAndReposition();
    },

    /**
     * Invoked when an app is clicked.
     * @param {Event} e The click event.
     * @private
     */
    onClick_: function(e) {
      var is_promo = this.appsPromoExtras_ &&
          window.getComputedStyle(this.appsPromoExtras_).display != 'none';
      var url = !this.appData_.is_webstore ? '' :
          is_promo ? this.appsPromoLink_.href :
                     appendParam(this.appData_.url,
                                 'utm_source',
                                 'chrome-ntp-icon');

      chrome.send('launchApp',
                  [this.appId, APP_LAUNCH.NTP_APPS_MAXIMIZED, url,
                   e.button, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey]);

      // Don't allow the click to trigger a link or anything
      e.preventDefault();
    },

    /**
     * Invoked when the user presses a key while the app is focused.
     * @param {Event} e The key event.
     * @private
     */
    onKeydown_: function(e) {
      if (e.keyIdentifier == 'Enter') {
        chrome.send('launchApp',
                    [this.appId, APP_LAUNCH.NTP_APPS_MAXIMIZED, '',
                     0, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey]);
        e.preventDefault();
        e.stopPropagation();
      }
    },

    /**
     * Adds a node to the list of targets that will launch the app. This list
     * is also used in onMousedown to determine whether the app contents should
     * be shown as active (if we don't do this, then clicking anywhere in
     * appContents, even a part that is outside the ideally clickable region,
     * will cause the app icon to look active).
     * @param {HTMLElement} node The node that should be clickable.
     */
    addLaunchClickTarget_: function(node) {
      node.classList.add('launch-click-target');
      node.addEventListener('click', this.onClick_.bind(this));
    },

    /**
     * Handler for mousedown on the App. Adds a class that allows us to
     * not display as :active for right clicks and clicks on app notifications
     * (specifically, don't pulse on these occasions). Also, we don't pulse
     * for clicks that aren't within the clickable regions.
     * @param {Event} e The mousedown event.
     */
    onMousedown_: function(e) {
      if (e.button == 2 ||
          !findAncestorByClass(e.target, 'launch-click-target')) {
        this.appContents_.classList.add('suppress-active');
      } else {
        this.appContents_.classList.remove('suppress-active');
      }

      // This class is here so we don't show the focus state for apps that
      // gain keyboard focus via mouse clicking.
      this.classList.add('click-focus');
    },

    /**
     * This app is losing keyboard focus.
     * @param {Event} e The event.
     */
    onBlur_: function(e) {
      this.classList.remove('click-focus');
    },

    /**
     * Change the appData and update the appearance of the app.
     * @param {Object} appData The new data object that describes the app.
     */
    replaceAppData: function(appData) {
      this.appData_ = appData;
      this.setIcon();
      this.loadIcon();
    },

    /**
     * The data and preferences for this app.
     * @type {Object}
     */
    set appData(data) {
      this.appData_ = data;
    },
    get appData() {
      return this.appData_;
    },

    get appId() {
      return this.appData_.id;
    },

    /**
     * Returns a pointer to the context menu for this app. All apps share the
     * singleton AppContextMenu. This function is called by the
     * ContextMenuHandler in response to the 'contextmenu' event.
     * @type {cr.ui.Menu}
     */
    get contextMenu() {
      var menu = AppContextMenu.getInstance();
      menu.setupForApp(this);
      return menu.menu;
    },

    /**
     * Returns whether this element can be 'removed' from chrome (i.e. whether
     * the user can drag it onto the trash and expect something to happen).
     * @return {boolean} True if the app can be uninstalled.
     */
    canBeRemoved: function() {
      return this.appData_.mayDisable;
    },

    /**
     * Uninstalls the app after it's been dropped on the trash.
     */
    removeFromChrome: function() {
      chrome.send('uninstallApp', [this.appData_.id, true]);
      this.tile.tilePage.removeTile(this.tile, true);
      if (this.currentBubbleShowing_)
        currentBubbleShowing_.hide();
    },

    /**
     * Called when a drag is starting on the tile. Updates dataTransfer with
     * data for this tile.
     */
    setDragData: function(dataTransfer) {
      dataTransfer.setData('Text', this.appData_.title);
      dataTransfer.setData('URL', this.appData_.url);
    },
  };

  var TilePage = ntp.TilePage;

  // The fraction of the app tile size that the icon uses.
  var APP_IMG_SIZE_FRACTION = 4 / 5;

  var appsPageGridValues = {
    // The fewest tiles we will show in a row.
    minColCount: 3,
    // The most tiles we will show in a row.
    maxColCount: 6,

    // The smallest a tile can be.
    minTileWidth: 64 / APP_IMG_SIZE_FRACTION,
    // The biggest a tile can be.
    maxTileWidth: 128 / APP_IMG_SIZE_FRACTION,

    // The padding between tiles, as a fraction of the tile width.
    tileSpacingFraction: 1 / 8,
  };
  TilePage.initGridValues(appsPageGridValues);

  /**
   * Creates a new AppsPage object.
   * @constructor
   * @extends {TilePage}
   */
  function AppsPage() {
    var el = new TilePage(appsPageGridValues);
    el.__proto__ = AppsPage.prototype;
    el.initialize();

    return el;
  }

  AppsPage.prototype = {
    __proto__: TilePage.prototype,

    initialize: function() {
      this.classList.add('apps-page');

      if (loadTimeData.getBoolean('appInstallHintEnabled')) {
        this.appInstallHint_ = $('app-install-hint-template').cloneNode(true);
        this.appInstallHint_.addEventListener('click', function(e) {
          chrome.send('recordAppLaunchByURL',
              [encodeURIComponent(this.href),
               APP_LAUNCH.NTP_WEBSTORE_PLUS_ICON]);
        });
        this.content_.appendChild(this.appInstallHint_);
      }

      this.addEventListener('cardselected', this.onCardSelected_);
      // Add event listeners for two events, so we can temporarily suppress
      // the app notification bubbles when the app card slides in and out of
      // view.
      this.addEventListener('carddeselected', this.onCardDeselected_);
      this.addEventListener('cardSlider:card_change_ended',
                            this.onCardChangeEnded_);

      this.addEventListener('tilePage:tile_added', this.onTileAdded_);

      this.content_.addEventListener('scroll', this.onScroll_.bind(this));
    },

    /**
     * Creates an app DOM element and places it at the last position on the
     * page.
     * @param {Object} appData The data object that describes the app.
     * @param {boolean=} animate If true, the app tile plays an animation.
     */
    appendApp: function(appData, animate) {
      if (animate) {
        // Select the page and scroll all the way down so the animation is
        // visible.
        ntp.getCardSlider().selectCardByValue(this);
        this.content_.scrollTop = this.content_.scrollHeight;
      }

      this.appendTile(new App(appData), animate);
      this.hintStateMayHaveChanged_();
    },

    /**
     * Similar to appendApp, but it respects the app_launch_ordinal field of
     * |appData|.
     * @param {Object} appData The data that describes the app.
     */
    insertApp: function(appData) {
      var index = 0;
      for (var i = 0; i < this.tileElements_.length; i++) {
        if (appData.app_launch_ordinal <
            this.tileElements_[i].firstChild.appData.app_launch_ordinal) {
          index = i;
          break;
        }
      }

      this.addTileAt(new App(appData), index, false);
      this.hintStateMayHaveChanged_();
    },

    /**
     * Handler for 'cardselected' event, fired when |this| is selected. The
     * first time this is called, we load all the app icons.
     * @private
     */
    onCardSelected_: function(e) {
      var apps = this.querySelectorAll('.app.icon-loading');
      for (var i = 0; i < apps.length; i++) {
        apps[i].loadIcon();
        if (apps[i].currentBubbleShowing_)
          apps[i].currentBubbleShowing_.suppressed = false;
      }
    },

    /**
     * Handler for tile additions to this page.
     * @param {Event} e The tilePage:tile_added event.
     */
    onTileAdded_: function(e) {
      assert(e.currentTarget == this);
      assert(e.addedTile.firstChild instanceof App);
      if (this.classList.contains('selected-card'))
        e.addedTile.firstChild.loadIcon();
    },

    /**
     * Handler for the when this.cardSlider ends change its card. If animated,
     * this happens when the -webkit-transition is done, otherwise happens
     * immediately (but after cardSlider:card_changed).
     * @private
     */
    onCardChangeEnded_: function(e) {
      for (var i = 0; i < this.tileElements_.length; i++) {
        var app = this.tileElements_[i].firstChild;
        assert(app instanceof App);
        if (app.currentBubbleShowing_)
          app.currentBubbleShowing_.suppressed = false;
      }
    },

    /**
     * Handler for the 'carddeselected' event, fired when the user switches
     * to another 'card' than the App 'card' on the NTP (|this| gets
     * deselected).
     * @private
     */
    onCardDeselected_: function(e) {
      for (var i = 0; i < this.tileElements_.length; i++) {
        var app = this.tileElements_[i].firstChild;
        assert(app instanceof App);
        if (app.currentBubbleShowing_)
          app.currentBubbleShowing_.suppressed = true;
      }
    },

    /**
     * A handler for when the apps page is scrolled (then we need to reposition
     * the bubbles.
     * @private
     */
    onScroll_: function(e) {
      if (!this.selected)
        return;
      for (var i = 0; i < this.tileElements_.length; i++) {
        var app = this.tileElements_[i].firstChild;
        assert(app instanceof App);
        if (app.currentBubbleShowing_)
          app.currentBubbleShowing_.resizeAndReposition();
        }
    },

    /** @inheritDoc */
    doDragOver: function(e) {
      var tile = ntp.getCurrentlyDraggingTile();
      if (tile && !tile.querySelector('.app')) {
        e.preventDefault();
        this.setDropEffect(e.dataTransfer);
      } else {
        TilePage.prototype.doDragOver.call(this, e);
      }
    },

    /** @inheritDoc */
    shouldAcceptDrag: function(e) {
      if (ntp.getCurrentlyDraggingTile())
        return true;
      if (!e.dataTransfer || !e.dataTransfer.types)
        return false;
      return Array.prototype.indexOf.call(e.dataTransfer.types,
                                          'text/uri-list') != -1;
    },

    /** @inheritDoc */
    addDragData: function(dataTransfer, index) {
      var sourceId = -1;
      var currentlyDraggingTile = ntp.getCurrentlyDraggingTile();
      if (currentlyDraggingTile) {
        var tileContents = currentlyDraggingTile.firstChild;
        if (tileContents.classList.contains('app')) {
          var originalPage = currentlyDraggingTile.tilePage;
          var samePageDrag = originalPage == this;
          sourceId = samePageDrag ? DRAG_SOURCE.SAME_APPS_PANE :
                                    DRAG_SOURCE.OTHER_APPS_PANE;
          this.tileGrid_.insertBefore(currentlyDraggingTile,
                                      this.tileElements_[index]);
          this.tileMoved(currentlyDraggingTile);
          if (!samePageDrag) {
            originalPage.fireRemovedEvent(currentlyDraggingTile, index, true);
            this.fireAddedEvent(currentlyDraggingTile, index, true);
          }
        } else if (currentlyDraggingTile.querySelector('.most-visited')) {
          this.generateAppForLink(tileContents.data);
          sourceId = DRAG_SOURCE.MOST_VISITED_PANE;
        }
      } else {
        this.addOutsideData_(dataTransfer);
        sourceId = DRAG_SOURCE.OUTSIDE_NTP;
      }

      assert(sourceId != -1);
      chrome.send('metricsHandler:recordInHistogram',
          ['NewTabPage.AppsPageDragSource', sourceId, DRAG_SOURCE_LIMIT]);
    },

    /**
     * Adds drag data that has been dropped from a source that is not a tile.
     * @param {Object} dataTransfer The data transfer object that holds drop
     *     data.
     * @private
     */
    addOutsideData_: function(dataTransfer) {
      var url = dataTransfer.getData('url');
      assert(url);

      // If the dataTransfer has html data, use that html's text contents as the
      // title of the new link.
      var html = dataTransfer.getData('text/html');
      var title;
      if (html) {
        // It's important that we don't attach this node to the document
        // because it might contain scripts.
        var node = this.ownerDocument.createElement('div');
        node.innerHTML = html;
        title = node.textContent;
      }

      // Make sure title is >=1 and <=45 characters for Chrome app limits.
      if (!title)
        title = url;
      if (title.length > 45)
        title = title.substring(0, 45);
      var data = {url: url, title: title};

      // Synthesize an app.
      this.generateAppForLink(data);
    },

    /**
     * Creates a new crx-less app manifest and installs it.
     * @param {Object} data The data object describing the link. Must have |url|
     *     and |title| members.
     */
    generateAppForLink: function(data) {
      assert(data.url != undefined);
      assert(data.title != undefined);
      var pageIndex = ntp.getAppsPageIndex(this);
      chrome.send('generateAppForLink', [data.url, data.title, pageIndex]);
    },

    /** @inheritDoc */
    tileMoved: function(draggedTile) {
      if (!(draggedTile.firstChild instanceof App))
        return;

      var pageIndex = ntp.getAppsPageIndex(this);
      chrome.send('setPageIndex', [draggedTile.firstChild.appId, pageIndex]);

      var appIds = [];
      for (var i = 0; i < this.tileElements_.length; i++) {
        var tileContents = this.tileElements_[i].firstChild;
        if (tileContents instanceof App)
          appIds.push(tileContents.appId);
      }

      chrome.send('reorderApps', [draggedTile.firstChild.appId, appIds]);
    },

    /** @inheritDoc */
    setDropEffect: function(dataTransfer) {
      var tile = ntp.getCurrentlyDraggingTile();
      if (tile && tile.querySelector('.app'))
        ntp.setCurrentDropEffect(dataTransfer, 'move');
      else
        ntp.setCurrentDropEffect(dataTransfer, 'copy');
    },

    /**
     * Called when we may need to change app install hint visibility.
     * @private
     */
    hintStateMayHaveChanged_: function() {
      if (this.updateHintState_())
        this.repositionTiles_();
      else
        this.repositionHint_();
    },

    /**
     * Updates whether the app install hint is visible. Returns true if we need
     * to reposition other tiles (because webstore app changed visibility).
     * @private
     */
    updateHintState_: function() {
      if (!this.appInstallHint_)
        return;

      var appsPages = document.querySelectorAll('.apps-page');
      var numTiles = this.tileElements_.length;
      var showHint =
          numTiles < appInstallHintTileLimit && appsPages.length == 1;
      this.appInstallHint_.hidden = !showHint;

      var webstoreApp = this.querySelector('.webstore');
      if (!webstoreApp)
        return false;

      var webstoreTile = findAncestorByClass(webstoreApp, 'tile');
      if (showHint) {
        if (!webstoreTile.classList.contains('real'))
          return false;

        webstoreTile.classList.remove('real');
        return true;
      }

      if (webstoreTile.classList.contains('real'))
        return false;

      webstoreTile.classList.add('real');
      return true;
    },

    /**
     * Repositions the app tile hint (to be called when tiles move).
     * @private
     */
    repositionHint_: function() {
      if (!this.appInstallHint_ || this.appInstallHint_.hidden)
        return;

      var index = this.tileElements_.length;
      var layout = this.layoutValues_;
      var col = index % layout.numRowTiles;
      var row = Math.floor(index / layout.numRowTiles);
      var realX = this.tileGrid_.offsetLeft +
          col * layout.colWidth + layout.leftMargin;

      var realY =
          this.topMarginPx_ + row * layout.rowHeight + this.contentPadding;

      this.appInstallHint_.style.left = realX + 'px';
      this.appInstallHint_.style.right = realX + 'px';
      this.appInstallHint_.style.top = realY + 'px';
      this.appInstallHint_.style.width = layout.tileWidth + 'px';
      this.appInstallHint_.style.height = layout.tileWidth + 'px';
    },

    /** @inheritDoc */
    repositionTiles_: function(ignoreNode) {
      TilePage.prototype.repositionTiles_.call(this, ignoreNode);
      this.repositionHint_();
    },
  };

  AppsPage.setPromo = function(data) {
    var store = document.querySelector('.webstore');
    if (store)
      store.setAppsPromoData(data);
  };

  /**
   * Launches the specified app using the APP_LAUNCH_NTP_APP_RE_ENABLE
   * histogram. This should only be invoked from the AppLauncherHandler.
   * @param {String} appID The ID of the app.
   */
  function launchAppAfterEnable(appId) {
    chrome.send('launchApp', [appId, APP_LAUNCH.NTP_APP_RE_ENABLE]);
  }

  function appNotificationChanged(id, notification) {
    var app = $(id);
    // The app might have been uninstalled, or notifications might be disabled.
    if (app && !app.appData.notifications_disabled)
      app.setupNotification_(notification);
  }

  return {
    APP_LAUNCH: APP_LAUNCH,
    appNotificationChanged: appNotificationChanged,
    AppsPage: AppsPage,
    launchAppAfterEnable: launchAppAfterEnable,
  };
});