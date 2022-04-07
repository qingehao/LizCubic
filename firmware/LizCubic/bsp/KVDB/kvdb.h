#ifndef __KVDB_H__
#define __KVDB_H__

int kvdb_init();

void kvdb_set_data(char *key, char *value);
int kvdb_get_data(char *key, char *value);
void kvdb_set_blob(char *key, void *value, int len);
int kvdb_get_blob(char *key, void *value, int len);

#endif
