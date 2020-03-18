#include "table.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

struct table *createTable(const int size)
{
	int i;
	struct table *t = (struct table *)malloc(sizeof(struct table));

	t->size = size;
	t->list = (struct node **)malloc(sizeof(struct node *)*size);

	for (i = 0; i < size; i++)
		t->list[i] = NULL;

	return t;
}

int hashCode(const struct table *t, const char *key)
{
	int i, hash = 0;

	for (i = 0; key[i] != 0; i++)
		hash = (hash ^ key[i]) % t->size;

	return hash;
}

void insert(struct table *t, const char *key, const char *val)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;
	struct node *newNode = (struct node *)malloc(sizeof(struct node));

	newNode->key = (char *)malloc(strlen(key) + 1);
	newNode->val = (char *)malloc(strlen(val) + 1);

	while (temp) {
		if (strcmp(temp->key, key) == 0) {
			strcpy(temp->val, val);
			return;
		}
		temp = temp->next;
	}
	strcpy(newNode->key, key);
	strcpy(newNode->val, val);
	newNode->next = list;
	t->list[pos] = newNode;
}

char *lookup(const struct table *t, const char *key)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;

	while (temp) {
		if (strcmp(temp->key, key) == 0)
			return temp->val;
		temp = temp->next;
	}

	return NULL;
}

void freeNode(struct node *n)
{
	free(n->key);
	free(n->val);
	free(n);
}

void removeKey(struct table *t, const char *key)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;
	struct node *next, *toFree;

	if (temp == NULL)
		return;

	if (strcmp(temp->key, key) == 0) {
		next = temp->next;
		freeNode(temp);
		t->list[pos] = next;
		return;
	}

	while (temp->next) {
		if (strcmp(temp->next->key, key) == 0)
		{
			toFree = temp->next;
			temp->next = temp->next->next;
			freeNode(toFree);
			break;
		}
		temp = temp->next;
	}
}

void destroyTable(struct table *t)
{
	int i;
	struct node *list;
	struct node *next;

	for (i = 0; i < t->size; i++) {
		list = t->list[i];
		while (list != NULL) {
			next = list->next;
			free(list->key);
			free(list->val);
			free(list);
			list = next;
		}
	}

	free(t->list);
	free(t);
}
