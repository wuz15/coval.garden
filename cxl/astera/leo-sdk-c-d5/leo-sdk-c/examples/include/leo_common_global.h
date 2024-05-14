#ifndef _COMMON_GLOBAL_H
#define _COMMON_GLOBAL_H

#define CLEAR_EVENTS 0
#define GET_EVENTS 1
#define GET_INTR_POLICY 2
#define SET_INTR_POLICY 3
#define GET_ALL_EVENTS 4
#define SET_ALERT_CFG 5
#define GET_ALERT_CFG 6

#define ALL_LOGS 2

#define ALERT_CFG_OT_MASK 2
#define ALERT_CFG_UT_MASK 4
#define ALERT_CFG_CVMEM_MASK 8
#define ALERT_CFG_ALL_MASK 0xE

#define ALERT_TEMP_MAX 125
#define ALERT_TEMP_MIN 0

void printevent(leo_one_evt_log_t *, int );
uint32_t populate_handles(char *str_handles, uint16_t *handles);

#endif
