#ifndef STORAGE_H
#define STORAGE_H

#include "net_profile.h"

bool storage_init();
bool storage_save_profile(const ConnectionProfile& profile);
bool storage_append_csv(const ConnectionProfile& profile);
bool storage_available();

#endif
