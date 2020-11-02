#ifndef HAVE_IN_FROM_DIR
#define HAVE_IN_FROM_DIR

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"
#include <map>

// Give a dir with in data, and translate these to ins in the program space. JCE, 2-11-2020
// dir contains the ins. prefix is prepended before the in's descriptors to avoid name collisions.
void read_ins_from_dir(const char* dir, const char* prefix);

#endif // HAVE_IN_FROM_DIR
