#include "tabs.h"

static void tab_cleanup(TabList *tabs) {
  if (tabs->current == NULL)
    return;
  ncplane_erase(nctabbed_content_plane(tabs->tb));
  if (tabs->current->destroyFn)
    tabs->current->destroyFn(tabs->current->data);
}

static void tab_next(TabList *tabs) {
  tab_cleanup(tabs);
  if (tabs->current->next != NULL)
    tabs->current = tabs->current->next;
  else
    tabs->current = tabs->head;
  nctabbed_select(tabs->tb, tabs->current->tab);
}

void tab_bindings(TabList *tabs, uint32_t key_id) {
  if (tabs->current->key_id == key_id)
    return;
  for (TabView *view = tabs->head; view != NULL; view = view->next)
    if (view->key_id == key_id) {
      tab_cleanup(tabs);
      tabs->current = view;
      nctabbed_select(tabs->tb, view->tab);
      return;
    }
  if (key_id == NCKEY_TAB)
    tab_next(tabs);
}
