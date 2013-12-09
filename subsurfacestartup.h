#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

#include "dive.h"
#include "divelist.h"
#include "libdivecomputer.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool imported;
extern bool has_pdftex;

void setup_system_prefs(void);
void parse_argument(const char *arg);

#ifdef __cplusplus
}
#endif

#endif
