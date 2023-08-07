#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "utils_daemon.h"

static int null_logger(const char *fmt, ...) { return 0; };

static const char *g_pidfile = NULL;
static const char *g_prog = "DAEMON_DEMO";
static int (*g_logger)(const char *fmt, ...) = null_logger;
static int (*g_service_start)() = NULL;
static void (*g_service_stop)() = NULL;

typedef int bool;
#define true 1
#define false 0

static pid_t first_parent_pid = 0;
static bool first_parent_quit = false;
static bool daemon_run = true;
static int child_exit_code = 0;

static void signal_handler(int signo)
{
	if (SIGINT == signo || SIGTSTP == signo)
	{
		g_logger(" program exit on interrupt, pid=%d\n", g_prog, getpid());
	}
	else
	{
		g_logger(" signal %s received. process pid %d terminated.\n",
				 g_prog, strsignal(signo), getpid());
	}
	exit(0);
}

static void signal_quit(int signo)
{
	if (g_service_stop)
	{
		g_logger(" child process receive SIGUSR1, invoke user's service-stop function!!!\n", g_prog);
		g_service_stop();
	}
}

static void init_sighandle()
{
	int i;
	for (i = 1; i <= 15; i++)
		sigset(i, signal_handler);
	sigset(SIGUSR1, signal_quit);
}

static void first_parent_on_signal_quit(int sig)
{
	first_parent_quit = 1; // set flag so that first parent will quit
}

static int daemon_init(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		// fork error
		return -1;
	}
	else if (pid != 0)
	{
		// parent process, wait for quit signal from grand-child
		signal(SIGTERM, first_parent_on_signal_quit);
		while (!first_parent_quit)
			usleep(100000);
		g_logger(" first parent (pid=%d) goes byebye, child (pid=%d) become a daemon now...\n",
				 g_prog, getpid(), pid);
		exit(0);
	}

	// child process, go on
	first_parent_pid = getppid();

	//	chdir("/");		// change current dir to /
	setsid(); // child goes on and become session leader
	umask(0);

	return 0;
}

void sa_childdead(int signo)
{
	int status;
	pid_t pid;

	// phoenix child process exit. should be due to unexpected program error. try to start it again
	while ((pid = waitpid(0, &status, WNOHANG)) > 0)
	{
		if (WIFSIGNALED(status))
		{
			g_logger(" service child process terminated by signal '%s'.\n",
					 g_prog, pid, strsignal(WTERMSIG(status)));
		}
		else if (WIFEXITED(status))
		{
			child_exit_code = WEXITSTATUS(status);
			g_logger(" service child (pid=%d) exited with code '%d'.\n", g_prog, pid, child_exit_code);
		}
	}
}

static void init_itimer(int which)
{
	struct itimerval tmr;

	tmr.it_value.tv_sec = 86400; // one full day. Shall be enough
	tmr.it_value.tv_usec = 0;
	tmr.it_interval.tv_sec = 86400;
	tmr.it_interval.tv_usec = 0;

	setitimer(which, &tmr, NULL);
}

static long get_itimer(int which)
{
	struct itimerval tmr;
	long delta_s, delta_us;

	getitimer(which, &tmr);
	delta_s = tmr.it_interval.tv_sec - tmr.it_value.tv_sec - 1;
	delta_us = 1000000 - tmr.it_value.tv_usec;
	if (delta_us == 1000000)
	{
		delta_s++;
		delta_us = 0;
	}
	// when elapsed time is very short, some time you get
	// negative delta_s. Zero it when it happened.
	if (delta_s < 0)
	{
		delta_s = 0;
		delta_us = 0;
	}
	return delta_s * 1000 + delta_us / 1000;
}

static void write_pidfile()
{
	// write my pid
	FILE *fp;
	fp = fopen(g_pidfile, "w");
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
}

static pid_t read_pidfile()
{
	FILE *fp;
	pid_t pid = 0;

	fp = fopen(g_pidfile, "r");
	if (fp != NULL)
	{
		fscanf(fp, "%d", &pid);
		fclose(fp);
	}
	return pid;
}

int daemon_config(const char *prog, const char *pidfile, int (*logger)(const char *fmt, ...))
{
	char path[512];
	char *ptr;
	struct stat st;

	if (prog)
		g_prog = prog;
	if (pidfile == NULL)
		return -1;
	strcpy(path, pidfile);
	if ((ptr = strrchr(path, '/')) != NULL)
		*ptr = '\0';
	if (stat(path, &st) == -1)
		return -1;
	g_pidfile = pidfile;
	g_logger = logger;
	return 0;
}

int daemon_service(int (*service_start)(), void (*service_stop)())
{
	g_service_start = service_start;
	g_service_stop = service_stop;
	return g_service_start != NULL && g_service_stop != NULL && g_pidfile != NULL;
}

static void sigterm_handle(int signo)
{
	//printf("SIGTERM is received, set quit flag!\n");
	// this signal is sent from other process that invoke daemon_stop to stop daemon and its child
	daemon_run = false;
}

static pid_t pid_child = 0;
int daemon_main(int run_as_daemon)
{
	struct sigaction chldsa;

	if (!run_as_daemon)
	{
		int rc;
		write_pidfile();
		g_logger(" =========== F O R G R O U N D   M O D E   S T A R T ============\n", g_prog);
		// ignore SIGPIPE
		bzero(&chldsa, sizeof(chldsa));
		chldsa.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &chldsa, NULL);
		// run service function
		rc = g_service_start();
		return rc;
	}

	daemon_init();
	g_logger(" =========== D A E M O N   M O D E   S T A R T ============\n", g_prog);
	// set signal to catch SIGCHLD
	// set signal handler for SIGTERM and mark my pid
	sigset(SIGTERM, sigterm_handle);
	// set SIGCHLD signal handler
	bzero(&chldsa, sizeof(chldsa));
	chldsa.sa_handler = &sa_childdead;
	sigemptyset(&chldsa.sa_mask);
	sigaction(SIGCHLD, &chldsa, NULL);
	write_pidfile();

phoenix_reborn:
	if ((pid_child = fork()) < 0)
	{
		// fork fail
		g_logger("��%s��phoenix child fork failed.\n", g_prog);
		exit(-1);
	}
	else if (pid_child != 0)
	{
		int msec_left;
		g_logger("��%s�� start a phoenix child process (pid=%d)\n", g_prog, pid_child);
		child_exit_code = 0;
		for (; daemon_run ;)
		{
			/*
			 * wait for 1 second. if child dead, I will be awaken and return from usleep call.
			 * but we have to wait for child to finish its signal handler and exit before we
			 * respawn a new one.
			 */
			init_itimer(ITIMER_REAL);
			msec_left = 1000;
			while (msec_left > 0 && kill(pid_child, 0) == 0)
			{
				usleep(msec_left * 1000);
				msec_left = 1000 - get_itimer(ITIMER_REAL);
			}

			if (!daemon_run)
				break;

			/*
			 * check for child has gone. If yes, respawn it
			 */
			if (kill(pid_child, 0) == -1)
			{
				if (child_exit_code != EDONOTRESPAWN)
				{
					g_logger("[Daemon Monitor] service process is dead or killed. respawn it.\n");
					goto phoenix_reborn;
				}
				else
				{
					g_logger("[Daemon Monitor] service process exit with STOP RESPAWN code - stop respawn and terminate!!!\n", child_exit_code);
					daemon_run = false;
				}
			}
		}
		// shutdown program, terminate working child
		// 1. notice working child process to quit
		kill(pid_child, SIGUSR1);
		// 2. wait until child process is terminated. 'waitpid' call will prevent child process stay in zoombie
		int i, rc_wpid = -1, status;
		for (i = 0; i < 10; i++)
		{
			usleep(10000); // wait 10 msec
			// check for engine process state. waitpid return >0 mean dead and in zombie now is leaved
			// 0 means child is still running or in zombie, -1 means that it's gone before waitpid
			if ((rc_wpid = waitpid(pid_child, &status, WNOHANG)) != 0)
				break;
			g_logger("[Daemon Monitor] service process (pid=%d) still exist, send SIGKILL...\n", pid_child);
			if (kill(pid_child, SIGKILL) == -1 && errno == ESRCH)
			{
				g_logger("--> service process is gone!\n");
				rc_wpid = -1;
				break;
			}
		}
		// remove pid file
		if (rc_wpid != -1)
		{
			g_logger("[GRAVE ERROR] %s service process refused to quit and cannot be killed!!!\n", g_prog);
		}
		unlink(g_pidfile);
	}
	else // [PHOENIX CHILD PROCESS]
	{
		// phoenix child process
		// inform first parent process to quit, we are a respawnable daemon now.
		if (first_parent_pid != 0)
		{
			kill(first_parent_pid, SIGTERM);
			first_parent_pid = 0;
		}
		g_logger("��%s�� phoenix (pid=%d)] service process up and run.\n", g_prog, getpid());
		// phoenix service process, who does the real work
		init_sighandle();
		// ignore SIGCHLD, over-ride parent signal setting on SIGCHLD
		bzero(&chldsa, sizeof(chldsa));
		chldsa.sa_handler = SIG_IGN;
		sigemptyset(&chldsa.sa_mask);
		sigaction(SIGCHLD, &chldsa, NULL);

		// invoke application's core service function. This is application core service and shall never return.
		if (g_service_start() == -1)
		{
			g_logger("��%s�� phoenix child service function return -1, donot respawn!\n", g_prog);
			exit(EDONOTRESPAWN);
		}
		g_logger("��%s�� phoenix (pid=%d)] service process exit.\n", g_prog, getpid());
		exit(0); // child process shall never return.
	}			 // end of PHOENIX CHILD Process
	return 0;
}

int daemon_stop()
{
	// signal running daemon process to quit
	pid_t pid;
	int rc = -1;

	if ((pid = read_pidfile()) != 0)
	{
		rc = kill(pid, SIGTERM);
	}
	return rc;
}

/*
 * probe is daemon process running
 */
int daemon_probe()
{
	pid_t pid = read_pidfile();
	return (pid != 0 && kill(pid, 0) == 0) ? 0 : -1;
}

/*
 * terminate child process guard by daemon parent
 */
int daemon_killchild()
{
	int rc = -1;
	if (pid_child != 0) // invoked by daemon process
	{
		g_logger("kill daemon working process, pid=%d...\n", pid_child);
		rc = kill(pid_child, SIGUSR1);
	}
	else // invoked by independent process
	{
		pid_t pid_daemon = read_pidfile();
		if (pid_daemon > 0)
		{
			printf("send SIGUSR1 to daemon monitor process pid=%d\n", pid_daemon);
			rc = kill(pid_daemon, SIGUSR1);
		}
	}
	return rc;
}

#ifdef ENABLE_DAEMON_TESTCODE
#include <stdarg.h>
#include "../utils/longtime.h"
#define SYSLOG_FILE "./systest.log"
#define PID_FILE "./test.pid"

static bool b_service_run;
static void service_run();
static void service_stop();

void lprintf(const char *fmt, ...)
{
	// log message in a peticular log file.
	FILE *fp = fopen(SYSLOG_FILE, "a");
	char buf[256];

	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	if (buf[0] == '\t')
	{
		fprintf(fp, "%24s%s", " ", buf + 1);
		printf("%24s%s", " ", buf + 1);
	}
	else
	{
		const char *strtime = TIMESTAMP();
		fprintf(fp, "[%s] %s", strtime, buf);
		printf("[%s] %s", strtime, buf);
	}
	fclose(fp);
}
int main(int argc, char *const argv[])
{
	int opt, i;
	bool b_stop = false;
	bool b_daemon = false;
	bool b_killchild = false;

	// parse run time argumnets
	while ((opt = getopt(argc, argv, ":dxk")) != -1)
	{
		switch (opt)
		{
		case ':':
			printf("missing required parameter for command option %c.\n", opt);
			return -1;
		case '?':
			printf("unknown option %c, ignored\n", optopt);
			break;
		case 'd':
			b_daemon = true;
			break;
		case 'k':
			b_killchild = true;
			break;
		case 'x':
			b_stop = true;
			break;
		}
	}

	daemon_config("DMTEST", PID_FILE, lprintf);
	daemon_service(service_run, service_stop);

	if (b_stop)
	{
		printf("Stop daemon and it's child process...\n");
		daemon_stop();
		return 0;
	}
	else if (b_killchild)
	{
		printf("Stop child process (the working process)...\n");
		daemon_killchild();
		return 0;
	}
	else if (daemon_probe() == 0)
	{
		printf("another copy is running, please stop it first by \"%s -x\"\n", argv[0]);
		return 0;
	}

	daemon_main(b_daemon);

	return 0;
}

static void service_run()
{
	srand(time(NULL));
	b_service_run = true;
	printf("daemon service function start working...\n");
	for (; b_service_run;)
	{
		// DO your service here
		if (rand() % 200 == 199)
		{
			printf("unrecoverable error, donot respawn me!\n");
			exit(EDONOTRESPAWN);
		}
		else if (rand() % 100 == 0)
		{
			printf("random error, please respawn me!\n");
			exit(0);
		}
		usleep(1000);
	}
	printf("daemon service function terminated!\n");
}

static void service_stop()
{
	printf("--> set flag to stop service function\n");
	b_service_run = false;
}

#endif
