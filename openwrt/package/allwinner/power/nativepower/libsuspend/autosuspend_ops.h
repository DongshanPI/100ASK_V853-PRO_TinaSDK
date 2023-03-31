/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBSUSPEND_AUTOSUSPEND_OPS_H_
#define _LIBSUSPEND_AUTOSUSPEND_OPS_H_

#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#endif

struct autosuspend_ops {
	int (*enable) (void);
	int (*disable) (void);
};

struct autosuspend_ops *autosuspend_autosleep_init(void);
struct autosuspend_ops *autosuspend_earlysuspend_init(void);
struct autosuspend_ops *autosuspend_wakeup_count_init(void);
struct autosuspend_ops *autosuspend_tinasuspend_init(void);

#endif
