
#ifndef __UUID_H
#define __UUID_H

typedef unsigned char uuid_t[16];

#if 0
typedef struct {
	unsigned char b[16];
} uuid_t;
#endif

void uuid_generate_random(uuid_t);
int uuid_parse(const char *uuid, uuid_t *u);
void my_uuid_unparse(uuid_t uu, char *out);
void uuid_unparse_upper(uuid_t uu, char *out);
void uuid_unparse_lower(uuid_t uu, char *out);

#endif
