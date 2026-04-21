#include <stdio.h>
#include <errno.h>
#include <stdint.h>

FILE* _Stdin  = NULL;
FILE* _Stdout = NULL;
FILE* _Stderr = NULL;

__attribute__((constructor))
static void qnx_shim_init(void) {
    _Stdin  = stdin;
    _Stdout = stdout;
    _Stderr = stderr;
}

int* __get_errno_ptr(void) {
    return &errno;
}

unsigned __stackavail(void) {
    return 1024 * 1024;
}

/*
 * Stack protection guard value.
 * On Linux glibc this is normally a TLS variable, but QNX expects a simple global.
 * We provide it here for the QNX static library objects.
 */
uintptr_t __stack_chk_guard __attribute__((weak)) = 0xDEADBEEF;

void __stack_chk_fail(void) __attribute__((weak));
void __stack_chk_fail(void) {
    /* should never trigger in the parser */
    __builtin_trap();
}

/*
 * QNX _Ctype table (ASCII-compatible subset).
 *
 * QNX ctype flags (from QNX <ctype.h>):
 *   _CTYPE_U  = 0x0001  uppercase
 *   _CTYPE_L  = 0x0002  lowercase
 *   _CTYPE_D  = 0x0004  digit
 *   _CTYPE_S  = 0x0008  whitespace
 *   _CTYPE_P  = 0x0010  punctuation
 *   _CTYPE_C  = 0x0020  control
 *   _CTYPE_X  = 0x0040  hex digit
 *   _CTYPE_B  = 0x0080  blank (space/tab)
 *   _CTYPE_A  = 0x0100  alpha
 *   _CTYPE_G  = 0x0200  graph
 *
 * Table has 257 entries: index 0 = EOF sentinel, indices 1..256 = chars 0..255.
 * The exported symbol _Ctype points to &table[1] so _Ctype[(unsigned char)c] works
 * and _Ctype[-1] = 0 (EOF sentinel).
 */
static const short ctype_table[257] = {
    /* index 0: EOF sentinel */
    0,
    /* chars 0..8: control */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 9 (TAB) */  0x0088,
    /* 10 (LF) */  0x0028,
    /* 11 (VT) */  0x0028,
    /* 12 (FF) */  0x0028,
    /* 13 (CR) */  0x0028,
    /* 14..31 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020,
    /* 32 (SPACE) */ 0x0088,
    /* 33 ! */  0x0210, /* 34 " */  0x0210, /* 35 # */  0x0210,
    /* 36 $ */  0x0210, /* 37 % */  0x0210, /* 38 & */  0x0210,
    /* 39 ' */  0x0210, /* 40 ( */  0x0210, /* 41 ) */  0x0210,
    /* 42 * */  0x0210, /* 43 + */  0x0210, /* 44 , */  0x0210,
    /* 45 - */  0x0210, /* 46 . */  0x0210, /* 47 / */  0x0210,
    /* 48..57: 0-9 (digit + hex + graph) */
    0x0244, 0x0244, 0x0244, 0x0244, 0x0244,
    0x0244, 0x0244, 0x0244, 0x0244, 0x0244,
    /* 58 : */  0x0210, /* 59 ; */  0x0210, /* 60 < */  0x0210,
    /* 61 = */  0x0210, /* 62 > */  0x0210, /* 63 ? */  0x0210,
    /* 64 @ */  0x0210,
    /* 65..70: A-F (upper + alpha + hex + graph) */
    0x0341, 0x0341, 0x0341, 0x0341, 0x0341, 0x0341,
    /* 71..90: G-Z (upper + alpha + graph) */
    0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301,
    0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301, 0x0301,
    0x0301, 0x0301, 0x0301, 0x0301,
    /* 91 [ */  0x0210, /* 92 \ */  0x0210, /* 93 ] */  0x0210,
    /* 94 ^ */  0x0210, /* 95 _ */  0x0210, /* 96 ` */  0x0210,
    /* 97..102: a-f (lower + alpha + hex + graph) */
    0x0342, 0x0342, 0x0342, 0x0342, 0x0342, 0x0342,
    /* 103..122: g-z (lower + alpha + graph) */
    0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302,
    0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302,
    0x0302, 0x0302, 0x0302, 0x0302,
    /* 123 { */ 0x0210, /* 124 | */ 0x0210, /* 125 } */ 0x0210,
    /* 126 ~ */ 0x0210,
    /* 127 DEL */ 0x0020,
    /* 128..255: high chars (zeroed for ASCII) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const short* _Ctype = &ctype_table[1];
