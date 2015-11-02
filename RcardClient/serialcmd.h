#ifndef SERIALCMD_H
#define SERIALCMD_H

enum SERIALCMD {
    /* client > board */
    READFRAME,
    SETDELAY,
    SETSPEEDDIV,
    GETID,

    /* board > client */
    UNKNOWNCMD,
    ACKSETDELAY,
    ACKSETSPEEDDIV,
    FRAMEDATA,
    CARDID
};

const int CMDLEN = 3;

const int GETID_ACK_SIZE = (1 + 10);
const int READFRAME_ACK_SIZE = (1 + 10 + 128 + 2 + 1);
const int DEFAULT_ACK_SIZE = 3;

#endif /* SERIALCMD_H */
