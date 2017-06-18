/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

// Transforms a (version, revision, patchlevel) triple into a number of the
// form 0xVVRRPPPP to allow comparing versions in a normalized way.
//
//See http://sourceforge.net/p/predef/wiki/VersionNormalization.
#define PROXC_CONFIG_VERSION(version, revision, patch) \
    (((version) << 24) + ((revision) << 16) + (patch))

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_MAJOR_VERSION 0

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_MINOR_VERSION 1

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_PATCH_VERSION 0

// Macro expanding to the full version of the library, in hexadecimal
// representation.
#define PROXC_VERSION                         \
    PROXC_CONFIG_VERSION(PROXC_MAJOR_VERSION, \
                         PROXC_MINOR_VERSION, \
                         PROXC_PATCH_VERSION)

