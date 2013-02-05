#ifndef INTTYPES_H
#define INTTYPES_H

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

#define INT16_MAX	0x7FFF
#define INT16_MIN	(-INT16_MAX-1)

#define INT32_MAX	0x7fffffffL
#define INT32_MIN	(-INT32_MAX-1L)

#define PRIX64	"llX"
#define PRIx64  "llx"
#define PRIu64  "llu"

#endif
