#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

void print_item(char *ptr, void *arg)
{
    fprintf(stderr, "[%s] ", ptr);
}

void remove_item(char *ptr, void *arg)
{
}

int main()
{
    GSList *list = NULL;

    list = g_slist_append(list, strdup("123"));
    list = g_slist_append(list, strdup("456"));
    list = g_slist_append(list, strdup("789"));
    list = g_slist_append(list, strdup("abc"));
    list = g_slist_append(list, strdup("def"));
    list = g_slist_append(list, strdup("hij"));

    fprintf(stderr, "phase1) list length=(%d)\n", g_slist_length(list));

    g_slist_foreach(list, (GFunc)print_item, NULL);
    fprintf(stderr, "\n");

    while (list != NULL && list->data != NULL) {
        fprintf(stderr, "%s\n", (char *)list->data);
        free(list->data);
        list = g_slist_remove(list, list->data);
    }

    fprintf(stderr, "phase2) list length=(%d)\n", g_slist_length(list));
    fprintf(stderr, "phase2) list ptr=(%s)\n", list == NULL ? "null" : "exist");
}
