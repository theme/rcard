#ifndef SERIALCMD_H
#define SERIALCMD_H

enum SERIALCMD {
    /* client > board */
    READ,
    SETDELAY,
    SETSPEEDDIV,
    TEST,

    /* board > client */
    CMDERROR,
    DATA
}

#endif /* SERIALCMD_H */


