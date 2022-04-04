/* Copyright 2011-2023 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>

static char *CTEST_FORMAT(char *buf, size_t n, const char* const fmt, ...) {
    if (buf) {
        buf[0] = '\0';
        va_list argp;
        va_start(argp, fmt);
        vsnprintf(buf, n, fmt, argp);
        va_end(argp);
    }
    return buf;
}

#ifdef __cplusplus
}
#endif
