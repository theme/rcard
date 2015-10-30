#ifndef SERIALCMD_H
#define SERIALCMD_H

enum SERIALCMD {
    /* CMD 3 byte,  CMD | ARG0 | ARG1 */ 
    CMDLEN = 3,
    /* client > board */
    READ,
    SETDELAY,
    SETSPEEDDIV,
    GETID,

    /* board > client */
    UNKNOWNCMD,
    ACKSETDELAY,
    ACKSETSPEEDDIV,
    DATA
};

#endif /* SERIALCMD_H */
