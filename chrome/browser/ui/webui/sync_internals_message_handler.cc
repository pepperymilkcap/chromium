// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sync_internals_message_handler.h"

#include <vector>

#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/about_sync_util.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "content/public/browser/web_ui.h"
#include "sync/internal_api/public/util/weak_handle.h"
#include "sync/js/js_arg_list.h"
#include "sync/js/js_event_details.h"

using syncer::JsArgList;
using syncer::JsEventDetails;
using syncer::JsReplyHandler;
using syncer::WeakHandle;

SyncInternalsMessageHandler::SyncInternalsMessageHandler()
    : weak_ptr_factory_(this) {}

SyncInternalsMessageHandler::~SyncInternalsMessageHandler() {
  if (js_controller_)
    js_controller_->RemoveJsEventHandler(this);
}

void SyncInternalsMessageHandler::RegisterMessages() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  // Init our link to the JsController.
  ProfileSyncService* service = GetProfileSyncService();
  if (service)
    js_controller_ = service->GetJsController();
  if (js_controller_)
    js_controller_->AddJsEventHandler(this);

  web_ui()->RegisterMessageCallback(
      "getAboutInfo",
      base::Bind(&SyncInternalsMessageHandler::OnGetAboutInfo,
                 base::Unretained(this)));

  RegisterJsControllerCallback("getNotificationState");
  RegisterJsControllerCallback("getNotificationInfo");
  RegisterJsControllerCallback("getRootNodeDetails");
  RegisterJsControllerCallback("getNodeSummariesById");
  RegisterJsControllerCallback("getNodeDetailsById");
  RegisterJsControllerCallback("getAllNodes");
  RegisterJsControllerCallback("getChildNodeIds");
  RegisterJsControllerCallback("getClientServerTraffic");
}

void SyncInternalsMessageHandler::OnGetAboutInfo(const base::ListValue* args) {
  // TODO(rlarocque): We should DCHECK(!args) here.
  scoped_ptr<base::DictionaryValue> value =
      sync_ui_util::ConstructAboutInformation(GetProfileSyncService());
  web_ui()->CallJavascriptFunction(
      "chrome.sync.getAboutInfo.handleReply",
      *value);
}

void SyncInternalsMessageHandler::HandleJsReply(
    const std::string& name, const JsArgList& args) {
  DVLOG(1) << "Handling reply for " << name << " message"
           << " with args " << args.ToString();
  const std::string& reply_handler = "chrome.sync." + name + ".handleReply";
  std::vector<const base::Value*> arg_list(args.Get().begin(),
                                           args.Get().end());
  web_ui()->CallJavascriptFunction(reply_handler, arg_list);
}

void SyncInternalsMessageHandler::HandleJsEvent(
    const std::string& name,
    const JsEventDetails& details) {
  DVLOG(1) << "Handling event: " << name
           << " with details " << details.ToString();
  const std::string& event_handler = "chrome.sync." + name + ".fire";
  std::vector<const base::Value*> arg_list(1, &details.Get());
  web_ui()->CallJavascriptFunction(event_handler, arg_list);
}

void SyncInternalsMessageHandler::RegisterJsControllerCallback(
    const std::string& name) {
  web_ui()->RegisterMessageCallback(
      name,
      base::Bind(&SyncInternalsMessageHandler::ForwardToJsController,
                 base::Unretained(this),
                 name));
}

void SyncInternalsMessageHandler::ForwardToJsController(
    const std::string& name,
    const base::ListValue* args) {
  if (js_controller_) {
    scoped_ptr<base::ListValue> args_copy(args->DeepCopy());
    JsArgList js_arg_list(args_copy.get());
    js_controller_->ProcessJsMessage(
        name, js_arg_list,
        MakeWeakHandle(weak_ptr_factory_.GetWeakPtr()));
  } else {
    DLOG(WARNING) << "No sync service; dropping message " << name;
  }
}

// Gets the ProfileSyncService of the underlying original profile.
// May return NULL (e.g., if sync is disabled on the command line).
ProfileSyncService* SyncInternalsMessageHandler::GetProfileSyncService() {
  Profile* profile = Profile::FromWebUI(web_ui());
  ProfileSyncServiceFactory* factory = ProfileSyncServiceFactory::GetInstance();
  return factory->GetForProfile(profile->GetOriginalProfile());
}

