// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webdata/web_intents_table.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "googleurl/src/gurl.h"
#include "sql/statement.h"

using webkit_glue::WebIntentServiceData;

namespace {

bool ExtractIntents(sql::Statement* s,
                    std::vector<WebIntentServiceData>* services) {
  DCHECK(s);
  if (!s->is_valid())
    return false;

  while (s->Step()) {
    WebIntentServiceData service;
    string16 tmp = s->ColumnString16(0);
    service.service_url = GURL(tmp);

    service.action = s->ColumnString16(1);
    service.type = s->ColumnString16(2);
    service.title = s->ColumnString16(3);
    tmp = s->ColumnString16(4);
     // Default to window disposition.
    service.disposition = WebIntentServiceData::DISPOSITION_WINDOW;
    if (tmp == ASCIIToUTF16("inline"))
      service.disposition = WebIntentServiceData::DISPOSITION_INLINE;
    services->push_back(service);
  }
  return s->Succeeded();
}

}  // namespace

WebIntentsTable::WebIntentsTable(sql::Connection* db,
                                 sql::MetaTable* meta_table)
    : WebDatabaseTable(db, meta_table) {
}

WebIntentsTable::~WebIntentsTable() {
}

bool WebIntentsTable::Init() {
  if (db_->DoesTableExist("web_intents"))
    return true;

  if (!db_->Execute("CREATE TABLE web_intents ("
                    "service_url LONGVARCHAR,"
                    "action VARCHAR,"
                    "type VARCHAR,"
                    "title VARCHAR,"
                    "disposition VARCHAR,"
                    "UNIQUE (service_url, action, type))")) {
    return false;
  }

  if (!db_->Execute("CREATE INDEX web_intents_index ON web_intents (action)"))
    return false;

  return true;
}

// TODO(jhawkins): Figure out Sync story.
bool WebIntentsTable::IsSyncable() {
  return false;
}

bool WebIntentsTable::GetWebIntentServices(
    const string16& action,
    std::vector<WebIntentServiceData>* services) {
  DCHECK(services);
  sql::Statement s(db_->GetUniqueStatement(
      "SELECT service_url, action, type, title, disposition FROM web_intents "
      "WHERE action=?"));
  s.BindString16(0, action);

  return ExtractIntents(&s, services);
}

// TODO(gbillock): This currently does a full-table scan. Eventually we will
// store registrations by domain, and so have an indexed origin. At that time,
// this should just change to do lookup by origin instead of URL.
bool WebIntentsTable::GetWebIntentServicesForURL(
    const string16& service_url,
    std::vector<WebIntentServiceData>* services) {
  DCHECK(services);
  sql::Statement s(db_->GetUniqueStatement(
      "SELECT service_url, action, type, title, disposition FROM web_intents "
      "WHERE service_url=?"));
  s.BindString16(0, service_url);

  return ExtractIntents(&s, services);
}

bool WebIntentsTable::GetAllWebIntentServices(
    std::vector<WebIntentServiceData>* services) {
  DCHECK(services);
  sql::Statement s(db_->GetUniqueStatement(
      "SELECT service_url, action, type, title, disposition FROM web_intents"));

  return ExtractIntents(&s, services);
}

bool WebIntentsTable::SetWebIntentService(const WebIntentServiceData& service) {
  sql::Statement s(db_->GetUniqueStatement(
      "INSERT OR REPLACE INTO web_intents "
      "(service_url, type, action, title, disposition) "
      "VALUES (?, ?, ?, ?, ?)"));

  // Default to window disposition.
  string16 disposition = ASCIIToUTF16("window");
  if (service.disposition == WebIntentServiceData::DISPOSITION_INLINE)
    disposition = ASCIIToUTF16("inline");
  s.BindString(0, service.service_url.spec());
  s.BindString16(1, service.type);
  s.BindString16(2, service.action);
  s.BindString16(3, service.title);
  s.BindString16(4, disposition);

  return s.Run();
}

// TODO(jhawkins): Investigate the need to remove rows matching only
// |service.service_url|. It's unlikely the user will be given the ability to
// remove at the granularity of actions or types.
bool WebIntentsTable::RemoveWebIntentService(
    const WebIntentServiceData& service) {
  sql::Statement s(db_->GetUniqueStatement(
      "DELETE FROM web_intents "
      "WHERE service_url = ? AND action = ? AND type = ?"));
  s.BindString(0, service.service_url.spec());
  s.BindString16(1, service.action);
  s.BindString16(2, service.type);

  return s.Run();
}
