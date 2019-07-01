//
// Created by idegani on 6/28/2019.
//
#include <liblfds711.h>
//#include <sys/queue.h>

#ifndef IDS_TYPES_H
#define IDS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct coordinate {
    int x;
    int y;
} coordinate;

typedef struct rect{
    int x;
    int y;
    int w;
    int h;
} rect;

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


typedef struct lfds711_queue_bss_state queue;
typedef struct lfds711_queue_bss_element q_element;
//int (*qr_enqueue)( struct lfds711_queue_bss_state *qbsss,void *key,void *value  ) = lfds711_queue_bss_enqueue;
#ifdef __cplusplus
}
#endif

#endif //IDS_TYPES_H
