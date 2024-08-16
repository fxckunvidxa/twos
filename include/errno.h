#pragma once

/* Argument list too long */
#define E2BIG 7
/* Permission denied */
#define EACCES 13
/* Address in use */
#define EADDRINUSE 98
/* Address not available */
#define EADDRNOTAVAIL 99
/* Address family not supported */
#define EAFNOSUPPORT 97
/* Resource unavailable, try again */
#define EAGAIN 11
/* Connection already in progress */
#define EALREADY 114
/* Bad file descriptor */
#define EBADF 9
/* Bad message */
#define EBADMSG 74
/* Device or resource busy */
#define EBUSY 16
/* Operation canceled */
#define ECANCELED 125
/* No child processes */
#define ECHILD 10
/* Connection aborted */
#define ECONNABORTED 103
/* Connection refused */
#define ECONNREFUSED 111
/* Connection reset */
#define ECONNRESET 104
/* Resource deadlock would occur */
#define EDEADLK 35
/* Destination address required */
#define EDESTADDRREQ 89
/* Mathematics argument out of domain of function */
#define EDOM 33
/* File exists */
#define EEXIST 17
/* Bad address */
#define EFAULT 14
/* File too large */
#define EFBIG 27
/* Host is unreachable */
#define EHOSTUNREACH 113
/* Identifier removed */
#define EIDRM 43
/* Illegal byte sequence */
#define EILSEQ 84
/* Operation in progress */
#define EINPROGRESS 115
/* Interrupted function */
#define EINTR 4
/* Invalid argument */
#define EINVAL 22
/* I/O error */
#define EIO 5
/* Socket is connected */
#define EISCONN 106
/* Is a directory */
#define EISDIR 21
/* Too many levels of symbolic links */
#define ELOOP 40
/* File descriptor value too large */
#define EMFILE 24
/* Too many links */
#define EMLINK 31
/* Message too large */
#define EMSGSIZE 90
/* Filename too long */
#define ENAMETOOLONG 36
/* Network is down */
#define ENETDOWN 100
/* Connection aborted by network */
#define ENETRESET 102
/* Network unreachable */
#define ENETUNREACH 101
/* Too many files open in system */
#define ENFILE 23
/* No buffer space available */
#define ENOBUFS 105
/* No message is available on the STREAM head read queue */
#define ENODATA 61
/* No such device */
#define ENODEV 19
/* No such file or directory */
#define ENOENT 2
/* Executable file format error */
#define ENOEXEC 8
/* No locks available */
#define ENOLCK 37
/* Link has been severed */
#define ENOLINK 67
/* Not enough space */
#define ENOMEM 12
/* No message of the desired type */
#define ENOMSG 42
/* Protocol not available */
#define ENOPROTOOPT 92
/* No space left on device */
#define ENOSPC 28
/* No STREAM resources */
#define ENOSR 63
/* Not a STREAM */
#define ENOSTR 60
/* Function not supported */
#define ENOSYS 38
/* The socket is not connected */
#define ENOTCONN 107
/* Not a directory */
#define ENOTDIR 20
/* Directory not empty */
#define ENOTEMPTY 39
/* State not recoverable */
#define ENOTRECOVERABLE 131
/* Not a socket */
#define ENOTSOCK 88
/* Not supported */
#define ENOTSUP 95
/* Inappropriate I/O control operation */
#define ENOTTY 25
/* No such device or address */
#define ENXIO 6
/* Operation not supported on socket */
#define EOPNOTSUPP 95
/* Value too large to be stored in data type */
#define EOVERFLOW 75
/* Previous owner died */
#define EOWNERDEAD 130
/* Operation not permitted */
#define EPERM 1
/* Broken pipe */
#define EPIPE 32
/* Protocol error */
#define EPROTO 71
/* Protocol not supported */
#define EPROTONOSUPPORT 93
/* Protocol wrong type for socket */
#define EPROTOTYPE 91
/* Result too large */
#define ERANGE 34
/* Read-only file system */
#define EROFS 30
/* Invalid seek */
#define ESPIPE 29
/* No such process */
#define ESRCH 3
/* Stream ioctl() timeout */
#define ETIME 62
/* Connection timed out */
#define ETIMEDOUT 110
/* Text file busy */
#define ETXTBSY 26
/* Operation would block */
#define EWOULDBLOCK 11
/* Cross-device link */
#define EXDEV 18
