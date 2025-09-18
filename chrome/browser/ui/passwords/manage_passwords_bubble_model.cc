// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/manage_passwords_bubble_model.h"

#include "chrome/browser/password_manager/password_store.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/passwords/manage_passwords_bubble_ui_controller.h"
#include "chrome/common/url_constants.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

using content::WebContents;
using autofill::PasswordFormMap;

ManagePasswordsBubbleModel::ManagePasswordsBubbleModel(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
  ManagePasswordsBubbleUIController* manage_passwords_bubble_ui_controller =
      ManagePasswordsBubbleUIController::FromWebContents(web_contents_);

  if (manage_passwords_bubble_ui_controller->password_to_be_saved())
    manage_passwords_bubble_state_ = PASSWORD_TO_BE_SAVED;
  else
    manage_passwords_bubble_state_ = MANAGE_PASSWORDS;

  title_ = l10n_util::GetStringUTF16(
      (manage_passwords_bubble_state_ == PASSWORD_TO_BE_SAVED) ?
          IDS_SAVE_PASSWORD : IDS_MANAGE_PASSWORDS);
  if (manage_passwords_bubble_ui_controller->password_to_be_saved()) {
    pending_credentials_ =
        manage_passwords_bubble_ui_controller->pending_credentials();
  }
  best_matches_ = manage_passwords_bubble_ui_controller->best_matches();
  manage_link_ =
      l10n_util::GetStringUTF16(IDS_OPTIONS_PASSWORDS_MANAGE_PASSWORDS_LINK);
}

ManagePasswordsBubbleModel::~ManagePasswordsBubbleModel() {}

void ManagePasswordsBubbleModel::OnCancelClicked() {
  manage_passwords_bubble_state_ = PASSWORD_TO_BE_SAVED;
}

void ManagePasswordsBubbleModel::OnSaveClicked() {
  ManagePasswordsBubbleUIController* manage_passwords_bubble_ui_controller =
      ManagePasswordsBubbleUIController::FromWebContents(web_contents_);
  manage_passwords_bubble_ui_controller->SavePassword();
}

void ManagePasswordsBubbleModel::OnManageLinkClicked() {
  chrome::ShowSettingsSubPage(chrome::FindBrowserWithWebContents(web_contents_),
                              chrome::kPasswordManagerSubPage);
}

void ManagePasswordsBubbleModel::OnCredentialAction(
    autofill::PasswordForm password_form,
    bool remove) {
  ManagePasswordsBubbleUIController* manage_passwords_bubble_ui_controller =
      ManagePasswordsBubbleUIController::FromWebContents(web_contents_);
  manage_passwords_bubble_ui_controller->OnCredentialAction(password_form,
                                                            remove);
}
