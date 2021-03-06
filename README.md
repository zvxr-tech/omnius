# OMNIUS

## Description


Omnius provides secure memory services to other processes. Specifically, it's current implementation provides 
the ability to constrain read/write access to the secure memory by way of a regular expression (E.g. "RW(WWR)*W").

Omnius communicates through an IPC Message Queue which is defined at omnius runtime. The IPC message type is used to tag the type of message that you want to deliver to omnius. Currently the following message types are
recognized:

    NIL       0x00
    LOAD      0x01
    UNLOAD    0x02
    ALLOC     0x03
    DEALLOC   0x04
    READ      0x05
    WRITE     0x06
    VIEW      0x07
    TERMINATE 0x08

The semantics of the message body is dependant on the message type. For more information on communicating with omnius, see: omnius/comm.h 


Although omnius operates independently, it is intended to be deployed on an external device (E.g. Rasberry Pi) and communicate with a host computer over USB, via a Facedancer board. For testing purposes a command line interface has been written (omnius-cli) which, when run on the same machine as omnius, allows you to communicate directly with omnius.


## Mechanics
To accomplish this you need three things.

1) A way to declare watched memory objects and a language to express policy about them.

2) A way to intercept memory events (I.e. read, write, execute) involving watched memory objects and intervene conditional on the applicable policy and state of that object.

3) A means to reason about the policy for a given memory object and memory event acting on it.

## Notes

- You can use the following command to clear a message queue for a given msg queue id

`ipcrm --queue-key 0xdead2`
