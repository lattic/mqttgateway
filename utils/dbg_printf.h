/*
 * dbg_printf.h
 *
 *  Created on: Sep 25, 2009
 *  Author: Richard Lee
 */

#ifndef DBG_PRINTF_H_
#define DBG_PRINTF_H_

#include <syslog.h>

#ifndef THIS_MODULE
#define THIS_MODULE "THIS_MODULE"
#endif

#define DBG_CONSOLE_DEV "/dev/console"

#define INFO_PRT(fmt, ...) do { \
		fprintf(stdout, "\033[0;32m" fmt "\033[0m\n", ##__VA_ARGS__); \
		syslog(LOG_INFO, fmt, ##__VA_ARGS__); \
	} while (0)

#define LOG_PRT(fmt, ...) do { \
		fprintf(stderr, "\033[0;37m" fmt "\033[0m\n", ##__VA_ARGS__); \
		syslog(LOG_INFO, fmt, ##__VA_ARGS__); \
	} while (0)

#define WARN_PRT(fmt, ...) do { \
		fprintf(stderr, "\033[0;33m" fmt "\033[0m\n", ##__VA_ARGS__); \
		syslog(LOG_WARNING, fmt, ##__VA_ARGS__); \
	} while (0)

#define CRIT_PRT(fmt, ...) do { \
		fprintf(stderr, "\033[0;31m" fmt "\033[0m\n", ##__VA_ARGS__); \
		syslog(LOG_CRIT, fmt, ##__VA_ARGS__); \
	} while (0)

#define ERR_PRT(fmt, ...) do { \
		fprintf(stderr, "\033[0;31m" fmt "\033[0m\n", ##__VA_ARGS__); \
		syslog(LOG_ERR, fmt, ##__VA_ARGS__); \
	} while (0)

#ifdef DBG_PRINTF
#define DBG_PRT(fmt, ...) do { \
		fprintf(stdout, fmt "\n", ##__VA_ARGS__); \
		syslog(LOG_DEBUG, fmt, ##__VA_ARGS__); \
	} while(0)

#define DBG_FUNC(fmt, ...) do { \
		fprintf(stdout, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__); \
		syslog(LOG_DEBUG, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
	} while(0)

#define ERR_FUNC(fmt, ...) do { \
		fprintf(stderr, "%s: \033[0;31m" fmt "\033[0m\n", __FUNCTION__, ##__VA_ARGS__); \
		syslog(LOG_ERR, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
	} while (0)
#else
#define DBG_PRT(fmt, ...) 
#define DBG_FUNC(fmt, ...) 
#define ERR_FUNC(fmt, ...) 
#endif

#endif /* DBG_PRINTF_H_ */
