#ifndef __SETUP_H__
#define __SETUP_H__

#define IDS_ENABLED         3000
#define IDS_ENABLED_REBOOT  3001
#define IDS_ENABLE_FAILED   3002
#define IDS_DISABLED        3003
#define IDS_DISABLED_REBOOT 3004
#define IDS_DISABLE_FAILED  3005
#define IDS_RESTARTED       3006
#define IDS_REQUIRES_REBOOT 3007
#define IDS_RESTART_FAILED  3008
#define IDS_REMOVED         3009
#define IDS_REMOVED_REBOOT  3010
#define IDS_REMOVE_FAILED   3011

//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)


#endif /*__SETUP_H__*/