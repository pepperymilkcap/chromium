// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/nss_profile_filter_chromeos.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/stringprintf.h"

namespace net {

namespace {

std::string CertSlotsString(const scoped_refptr<X509Certificate>& cert) {
  std::string result;
  crypto::ScopedPK11SlotList slots_for_cert(
      PK11_GetAllSlotsForCert(cert->os_cert_handle(), NULL));
  for (PK11SlotListElement* slot_element =
           PK11_GetFirstSafe(slots_for_cert.get());
       slot_element;
       slot_element =
           PK11_GetNextSafe(slots_for_cert.get(), slot_element, PR_FALSE)) {
    if (!result.empty())
      result += ',';
    base::StringAppendF(&result,
                        "%lu:%lu",
                        PK11_GetModuleID(slot_element->slot),
                        PK11_GetSlotID(slot_element->slot));
  }
  return result;
}

}  // namespace

NSSProfileFilterChromeOS::NSSProfileFilterChromeOS() {}

NSSProfileFilterChromeOS::~NSSProfileFilterChromeOS() {}

void NSSProfileFilterChromeOS::Init(crypto::ScopedPK11Slot public_slot,
                                    crypto::ScopedPK11Slot private_slot) {
  public_slot_ = public_slot.Pass();
  private_slot_ = private_slot.Pass();
}

bool NSSProfileFilterChromeOS::IsModuleAllowed(PK11SlotInfo* slot) const {
  // If this is one of the public/private slots for this profile, allow it.
  if (slot == public_slot_.get() || slot == private_slot_.get())
    return true;
  // If it's from the read-only slot, allow it.
  if (PK11_IsInternalKeySlot(slot))
    return true;
  // If this is not the internal (file-system) module or the TPM module, allow
  // it.
  SECMODModule* module_for_slot = PK11_GetModule(slot);
  if (module_for_slot != PK11_GetModule(public_slot_.get()) &&
      module_for_slot != PK11_GetModule(private_slot_.get()))
    return true;
  return false;
}

bool NSSProfileFilterChromeOS::IsCertAllowed(
    const scoped_refptr<X509Certificate>& cert) const {
  crypto::ScopedPK11SlotList slots_for_cert(
      PK11_GetAllSlotsForCert(cert->os_cert_handle(), NULL));
  if (!slots_for_cert) {
    DVLOG(2) << "cert no slots: " << cert->subject().GetDisplayName();
    return true;
  }

  for (PK11SlotListElement* slot_element =
           PK11_GetFirstSafe(slots_for_cert.get());
       slot_element;
       slot_element =
           PK11_GetNextSafe(slots_for_cert.get(), slot_element, PR_FALSE)) {
    if (IsModuleAllowed(slot_element->slot)) {
      DVLOG(3) << "cert from " << CertSlotsString(cert)
               << " allowed: " << cert->subject().GetDisplayName();
      PK11_FreeSlotListElement(slots_for_cert.get(), slot_element);
      return true;
    }
  }
  DVLOG(2) << "cert from " << CertSlotsString(cert)
           << " filtered: " << cert->subject().GetDisplayName();
  return false;
}

NSSProfileFilterChromeOS::CertNotAllowedForProfilePredicate::
    CertNotAllowedForProfilePredicate(const NSSProfileFilterChromeOS& filter)
    : filter_(filter) {}

bool NSSProfileFilterChromeOS::CertNotAllowedForProfilePredicate::operator()(
    const scoped_refptr<X509Certificate>& cert) const {
  return !filter_.IsCertAllowed(cert);
}

NSSProfileFilterChromeOS::ModuleNotAllowedForProfilePredicate::
    ModuleNotAllowedForProfilePredicate(const NSSProfileFilterChromeOS& filter)
    : filter_(filter) {}

bool NSSProfileFilterChromeOS::ModuleNotAllowedForProfilePredicate::operator()(
    const scoped_refptr<CryptoModule>& module) const {
  return !filter_.IsModuleAllowed(module->os_module_handle());
}

}  // namespace net

