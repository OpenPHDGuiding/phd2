#ifndef _IO_INDI_H_
#define _IO_INDI_H_
#ifdef __cplusplus
extern "C" {
#endif

extern int io_indi_sock_read(void *fh, void *data, int len);
extern int io_indi_sock_write(void *fh, void *data, int len);
extern void *io_indi_open_server(const char *host, int port, void (*cb)(void *fd, void *opaque), void *opaque);
extern void io_indi_idle_callback(int (*cb)(void *data), void *data);
extern void io_indi_close_server(void *fh);
  
#ifdef __cplusplus
}
#endif
#endif //_IO_INDI_H_
