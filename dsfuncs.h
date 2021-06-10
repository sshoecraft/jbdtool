
#ifndef __DEVSERVER_FUNCS_H
#define __DEVSERVER_FUNCS_H

int devserver_send(int fd, uint8_t opcode, uint8_t unit, void *data, int datasz);
int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout);
int devserver_request(int fd, uint8_t opcode, uint8_t unit, void *data, int len);
int devserver_reply(int fd, uint8_t status, uint8_t unit ,uint8_t *data, int len);
int devserver_error(int fd, uint8_t status);

#define devserver_getshort(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_putshort(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_put16(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get32(p) (uint16_t)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))
#define devserver_put32(p,v) *(p) = ((int)(v) & 0xFF); *((p)+1) = ((int)(v) >> 8) & 0xFF; *((p)+2) = ((int)(v) >> 16) & 0xFF; *((p)+3) = ((int)(v) >> 24) & 0xFF

#endif /* __DEVSERVER_FUNCS_H */
