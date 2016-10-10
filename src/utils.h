
#ifndef UTILS_H__
#define UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <slog.h>

/* 
 * LOG configuring with SLOG
 */
#ifdef DEBUG
#   define log_init(...) slog_init(__VA_ARGS__)
#   define log(...)      slog(__VA_ARGS__)
#else
#   define log_init(...) /* do nothing */
#   define log(...)      /* do nothing */
#endif

#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

#define ESC "\x1b["

#define BLACK(txt)   ESC "30;1m" txt ESC "0m"
#define RED(txt)     ESC "31;1m" txt ESC "0m"
#define GREEN(txt)   ESC "32;1m" txt ESC "0m"
#define YELLOW(txt)  ESC "33;1m" txt ESC "0m"
#define BLUE(txt)    ESC "34;1m" txt ESC "0m"
#define MAGENTA(txt) ESC "35;1m" txt ESC "0m"
#define CYAN(txt)    ESC "36;1m" txt ESC "0m"
#define WHITE(txt)   ESC "37;1m" txt ESC "0m"

#define INFO_MSG "%s:%d:%s(): ", __FILE__, __LINE__, __func__ 

/*
 * DEBUG function
 */
#ifdef DEBUG
#   define DEBUG_PRINT(...) \
        fprintf(stderr, BLUE("DEBUG") ": " __FILE__ ":" STRINGIFY(__LINE__) \
                ":" STRINGIFY(__func__) "(): " __VA_ARGS__)
#else
#   define DEBUG_PRINT(...) /* don't do anything */
#endif

/* 
 * ASSERT function
 */
#ifdef DEBUG
#   define ASSERT(cond, ...) \
        do { \
            if (!(cond)) { \
                fprintf(stderr, RED("ASSERT") ": " __FILE__ ":" STRINGIFY(__LINE__) \
                        ":" STRINGIFY(__func__) "(): " __VA_ARGS__); \
                abort(); \
            } \
        } while (0)

#else
#   define ASSERT(cond, ...) /* don't do anything */
#endif

/*
 * EXIT function
 */
#define EXIT(status, ...) \
    do { \
        fprintf(stderr, RED("EXIT") ": " __FILE__ ":" \
                STRINGIFY(__LINE__) ":" STRINGIFY(__func__) "(): " \
                __VA_ARGS__); \
        exit(status); \
    } while (0)

#endif /* UTILS_H__ */

