#include <stdlib.h>
#include <string.h>

struct node {
	char *key;
	char *val;
	struct node *next;
};

struct table {
	int size;
	struct node **list;
};

struct table *createTable(int size);

int hashCode(const struct table *t, const char *key);

void insert(struct table *t, const char *key, const char *val);

void removeKey(struct table *t, const char *key);

char *lookup(const struct table *t, const char *key);

void destroyTable(struct table *t);
