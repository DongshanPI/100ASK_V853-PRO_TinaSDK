#ifndef __AACTD_DRC_HW_H__
#define __AACTD_DRC_HW_H__

#include "aactd/communicate.h"

#ifndef DRC_HW_REG_BASE_ADDR
#error "not define DRC_HW_REG_BASE_ADDR"
#endif

int drc_hw_local_init(void);
int drc_hw_local_release(void);

int drc_hw_write_com_to_local(const struct aactd_com *com);

#endif /* ifndef __AACTD_DRC_HW_H__ */
