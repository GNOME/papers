// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2012 Christian Persch
 */

#pragma once

#if defined(PPS_DISABLE_DEPRECATION_WARNINGS)
#define PPS_DEPRECATED
#define PPS_DEPRECATED_FOR(f)
#define PPS_UNAVAILABLE(maj, min)
#else
#define PPS_DEPRECATED G_DEPRECATED
#define PPS_DEPRECATED_FOR(f) G_DEPRECATED_FOR (f)
#define PPS_UNAVAILABLE(maj, min) G_UNAVAILABLE (maj, min)
#endif

#ifdef __has_attribute
#if __has_attribute(__visibility__)
#define PPS_PUBLIC __attribute__ ((__visibility__ ("default"))) extern
#endif
#endif
#ifndef PPS_PUBLIC
#define PPS_PUBLIC extern
#endif

#define PPS_PRIVATE PPS_PUBLIC
