// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/tray_power.h"

#include "ash/shell.h"
#include "ash/system/date/date_view.h"
#include "ash/system/power/power_supply_status.h"
#include "ash/system/tray/system_tray_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_views.h"
#include "base/string_number_conversions.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "grit/ash_strings.h"
#include "grit/ui_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/size.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "unicode/fieldpos.h"
#include "unicode/fmtable.h"

namespace ash {
namespace internal {

namespace {
// Width and height of battery images.
const int kBatteryImageHeight = 25;
const int kBatteryImageWidth = 25;
// Number of different power states.
const int kNumPowerImages = 15;
// Top/bottom padding of the text items.
const int kPaddingVertical = 10;
}

namespace tray {

// This view is used only for the tray.
class PowerTrayView : public views::ImageView {
 public:
  PowerTrayView() {
    UpdateImage();
  }

  virtual ~PowerTrayView() {
  }

  void UpdatePowerStatus(const PowerSupplyStatus& status) {
    supply_status_ = status;
    // Sanitize.
    if (supply_status_.battery_is_full)
      supply_status_.battery_percentage = 100.0;

    UpdateImage();
    SetVisible(status.battery_is_present);
  }

 private:
  void UpdateImage() {
    SkBitmap image;
    gfx::Image all = ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_AURA_UBER_TRAY_POWER_SMALL);

    int image_index = 0;
    if (supply_status_.battery_percentage >= 100) {
      image_index = kNumPowerImages - 1;
    } else if (!supply_status_.battery_is_present) {
      image_index = kNumPowerImages;
    } else {
      image_index = static_cast<int> (
          supply_status_.battery_percentage / 100.0 *
          (kNumPowerImages - 1));
      image_index =
        std::max(std::min(image_index, kNumPowerImages - 2), 0);
    }

    // TODO(mbolohan): Remove the 2px offset when the assets are centered. See
    // crbug.com/119832.
    SkIRect region = SkIRect::MakeXYWH(
        (supply_status_.line_power_on ? kBatteryImageWidth : 0) + 2,
        image_index * kBatteryImageHeight,
        kBatteryImageWidth - 2, kBatteryImageHeight);
    all.ToSkBitmap()->extractSubset(&image, region);

    SetImage(image);
  }

  PowerSupplyStatus supply_status_;

  DISALLOW_COPY_AND_ASSIGN(PowerTrayView);
};

// This view is used only for the popup.
class PowerPopupView : public views::View {
 public:
  PowerPopupView() {
    status_label_ = new views::Label;
    status_label_->SetHorizontalAlignment(views::Label::ALIGN_RIGHT);
    time_label_ = new views::Label;
    time_label_->SetHorizontalAlignment(views::Label::ALIGN_RIGHT);
    UpdateText();

    SetLayoutManager(
        new views::BoxLayout(
            views::BoxLayout::kVertical, 0, 0, kTrayPopupTextSpacingVertical));
    AddChildView(status_label_);
    AddChildView(time_label_);
  }

  virtual ~PowerPopupView() {
  }

  void UpdatePowerStatus(const PowerSupplyStatus& status) {
    supply_status_ = status;
    // Sanitize.
    if (supply_status_.battery_is_full)
      supply_status_.battery_percentage = 100.0;

    UpdateText();
  }

 private:
  void UpdateText() {
    if (supply_status_.is_calculating_battery_time) {
      status_label_->SetText(
          l10n_util::GetStringFUTF16(
              IDS_ASH_STATUS_TRAY_BATTERY_PERCENT,
              base::IntToString16(
                  static_cast<int>(supply_status_.battery_percentage))));
      time_label_->SetText(
          ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
              supply_status_.line_power_on ?
                  IDS_ASH_STATUS_TRAY_BATTERY_CALCULATING_ON :
                  IDS_ASH_STATUS_TRAY_BATTERY_CALCULATING_OFF));
      return;
    }

    base::TimeDelta time = base::TimeDelta::FromSeconds(
        supply_status_.line_power_on ?
        supply_status_.battery_seconds_to_full :
        supply_status_.battery_seconds_to_empty);
    int hour = time.InHours();
    int min = (time - base::TimeDelta::FromHours(hour)).InMinutes();
    if (hour || min) {
      status_label_->SetText(
          l10n_util::GetStringFUTF16(
              IDS_ASH_STATUS_TRAY_BATTERY_PERCENT,
              base::IntToString16(
                  static_cast<int>(supply_status_.battery_percentage))));
      time_label_->SetText(
          l10n_util::GetStringFUTF16(
              supply_status_.line_power_on ?
                  IDS_ASH_STATUS_TRAY_BATTERY_TIME_UNTIL_FULL :
                  IDS_ASH_STATUS_TRAY_BATTERY_TIME_UNTIL_EMPTY,
              base::IntToString16(hour),
              base::IntToString16(min)));
    } else {
      if (supply_status_.line_power_on) {
        status_label_->SetText(
            ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
                IDS_ASH_STATUS_TRAY_BATTERY_FULL));
      } else {
        // Completely discharged? ... ha?
        status_label_->SetText(string16());
      }
      time_label_->SetText(string16());
    }
  }

  views::Label* status_label_;
  views::Label* time_label_;

  PowerSupplyStatus supply_status_;

  DISALLOW_COPY_AND_ASSIGN(PowerPopupView);
};

}  // namespace tray

TrayPower::TrayPower()
    : date_(NULL),
      power_(NULL),
      power_tray_(NULL) {
}

TrayPower::~TrayPower() {
}

views::View* TrayPower::CreateTrayView(user::LoginStatus status) {
  // There may not be enough information when this is created about whether
  // there is a battery or not. So always create this, and adjust visibility as
  // necessary.
  PowerSupplyStatus power_status =
      ash::Shell::GetInstance()->tray_delegate()->GetPowerSupplyStatus();
  CHECK(power_tray_ == NULL);
  power_tray_ = new tray::PowerTrayView();
  power_tray_->UpdatePowerStatus(power_status);
  return power_tray_;
}

views::View* TrayPower::CreateDefaultView(user::LoginStatus status) {
  CHECK(date_ == NULL);
  date_ = new tray::DateView();

  views::View* container = new views::View;
  views::BoxLayout* layout = new views::BoxLayout(views::BoxLayout::kHorizontal,
      0, 0, 0);

  layout->set_spread_blank_space(true);
  container->SetLayoutManager(layout);
  container->set_background(views::Background::CreateSolidBackground(
      kHeaderBackgroundColor));
  HoverHighlightView* view = new HoverHighlightView(NULL);
  view->SetLayoutManager(new views::FillLayout);
  view->AddChildView(date_);
  date_->set_border(views::Border::CreateEmptyBorder(kPaddingVertical,
      kTrayPopupPaddingHorizontal,
      kPaddingVertical,
      kTrayPopupPaddingHorizontal));
  container->AddChildView(view);
  view->set_focusable(false);

  if (status != user::LOGGED_IN_NONE && status != user::LOGGED_IN_LOCKED) {
    date_->SetActionable(true);
    view->set_highlight_color(kHeaderHoverBackgroundColor);
  } else {
    view->set_highlight_color(SkColorSetARGB(0, 0, 0, 0));
  }

  PowerSupplyStatus power_status =
      ash::Shell::GetInstance()->tray_delegate()->GetPowerSupplyStatus();
  if (power_status.battery_is_present) {
    CHECK(power_ == NULL);
    power_ = new tray::PowerPopupView();
    power_->UpdatePowerStatus(power_status);
    power_->set_border(views::Border::CreateSolidSidedBorder(
        kPaddingVertical, kTrayPopupPaddingHorizontal,
        kPaddingVertical, kTrayPopupPaddingHorizontal,
        SkColorSetARGB(0, 0, 0, 0)));
    container->AddChildView(power_);
  }
  ash::Shell::GetInstance()->tray_delegate()->RequestStatusUpdate();

  return container;
}

views::View* TrayPower::CreateDetailedView(user::LoginStatus status) {
  return NULL;
}

void TrayPower::DestroyTrayView() {
  power_tray_ = NULL;
}

void TrayPower::DestroyDefaultView() {
  date_ = NULL;
  power_ = NULL;
}

void TrayPower::DestroyDetailedView() {
}

void TrayPower::UpdateAfterLoginStatusChange(user::LoginStatus status) {
}

void TrayPower::OnPowerStatusChanged(const PowerSupplyStatus& status) {
  if (power_tray_)
    power_tray_->UpdatePowerStatus(status);
  if (power_)
    power_->UpdatePowerStatus(status);
}

}  // namespace internal
}  // namespace ash
