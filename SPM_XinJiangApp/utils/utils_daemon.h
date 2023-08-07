#ifndef _UTILS_DAEMON_INCLUDED_
#define _UTILS_DAEMON_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif
#include <errno.h>
/* 
 * config the daemon basic information
 * 'pidfile' is the file path to record the process pid. It's better be a RAM file. therefore
 * we can ensure after power cycle, this file won't exist.
 * 'prog' is the program name like "LPNR"
 * 'logger' is application provided logger function. If NULL is provided, no log will be written.
 */
int daemon_config(const char *pidfile, const char *prog, int (*logger)(const char *fmt,...));

/*
 * provide a core service function.
 * argument 'service_start' is the core service function provided by application.
 * 'service_stop' is a function which set a flag to stop the serivce function main loop.
 * these two functions will be invoked by daemon_main.
 * 'service_start' shall never return unless stop by 'service_stop'
 * however, if 'service_start' return -1, daemon_main will not respawn
 * as it indicate that service start-up failure. It is responsibility of user's 'service_start' 
 * to log detail of failure.
 */
int daemon_service( int (*service_start)(), void (*service_stop)() );

/*
 * initiate the daemon mechanism and invoke application specified core service function.
 * 
 * make sure 'daemon_config' and 'daemon_service' are invoked before invoke daemon_main
 *
 * NOTE - daemon_main never return, unless stop by daemon_stop who will issue SIGTERM to
 *        stop daemon process
 *
 * 'run_as_daemon' 1 means run daemon, service process is run as child and will
 * be respawn whenever it's gone. unless it is exit with error (respawn will exit again)
 * 'run_as_daemon' 0 means run on forground. 
 */
int daemon_main(int run_as_daemon);

/*
 * stop the other process run as daemon which its pid is written in file given in first argument
 * of daemin_config
 */
int daemon_stop();

/*
 * probe if the other copy of process is running
 * return 0: yes
 * -1 no.
 */
int daemon_probe();

/* 
 * terminate child process guard by daemon parent. canbe invoked by daemon parent
 * or by another process. But daemon_config must be invoked properly before this 
 * function is invoked.
 */
int daemon_killchild();

// when child process exit with this code. daemon parent won't restart it.
// this is used when a fatal error cannot recovery by restart service process
#define EDONOTRESPAWN	 ENOTRECOVERABLE	

#ifdef __cplusplus
};
#endif


#endif
