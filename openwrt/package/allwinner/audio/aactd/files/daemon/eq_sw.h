#ifndef __AACTD_EQ_SW_H__
#define __AACTD_EQ_SW_H__

#include "aactd/communicate.h"

int eq_sw_local_init(void);
int eq_sw_local_release(void);

int eq_sw_write_com_to_local(const struct aactd_com *com);

#endif /* ifndef __AACTD_EQ_SW_H__ */
