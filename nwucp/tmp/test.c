#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

void print_item(char *ptr, void *arg)
{
    fprintf(stderr, "[%s] ", ptr);
}

gint compare_item(gpointer data, gpointer arg)
{
	const char *item_data = (const char *)data;
	const char *find = (char *)arg;
	fprintf(stderr, "%s() called, [%s] [%s]\n", __func__, item_data, find);

	return (strcmp(item_data, find));
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

	GSList *item = g_slist_find_custom(list, "abc", (GCompareFunc)compare_item);
	if (item != NULL) {
		fprintf(stderr, "find item! will remove\n");
		free(item->data);
		list = g_slist_remove(list, item->data);
	}

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
