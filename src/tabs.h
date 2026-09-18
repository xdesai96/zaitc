#ifndef ZAITC_TABS
#define ZAITC_TABS

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

typedef struct TabView {
  struct nctab *tab;
  unsigned int key_id;
  void *data;
  void (*destroyFn)(void *data);
  struct TabView *next;
} TabView;

typedef struct {
  TabView *head;
  TabView *current;
  struct nctabbed *tb;
} TabList;

void tab_bindings(TabList *tabs, uint32_t key_id);

#endif // ZAITC_TABS
