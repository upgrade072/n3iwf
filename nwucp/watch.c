#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;

//// API

#define NOTIFY_WATCH_MAX_BUFF (1024 * 12)
static void (*directory_watch_action)();
static void config_watch_callback(struct bufferevent *bev, void *args)
{
    char buf[NOTIFY_WATCH_MAX_BUFF] = {0,};
    size_t numRead = bufferevent_read(bev, buf, NOTIFY_WATCH_MAX_BUFF);
    char *ptr;
    for (ptr = buf; ptr < buf + numRead; ) {
        struct inotify_event *event = (struct inotify_event*)ptr;
        if (directory_watch_action != NULL && event->len > 0) {
            /* function point */
            directory_watch_action(event->name);
        }
        ptr += sizeof(struct inotify_event) + event->len;
    }
}

int watch_directory_init(struct event_base *evbase, const char *path_name, void (*callback_function)(const char *arg_is_path))
{
    if (callback_function == NULL) {
        fprintf(stderr, "INIT| ERR| callback function is NULL!");
        return -1;
    }

    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        fprintf(stderr, "INIT| ERR| inotify init fail!\n");
        return -1;
    }

    int inotifyWd = inotify_add_watch(inotifyFd, path_name, IN_CLOSE_WRITE | IN_MOVED_TO);
    if (inotifyWd == -1) {
        fprintf(stderr, "INIT| ERR| inotify add watch [%s] fail!\n", path_name);
        return -1;
    } else {
        fprintf(stderr, "INIT| inotify add watch [%d:%s]\n", inotifyWd, path_name);

        /* ADD CALLBACK ACTION */
        directory_watch_action = callback_function;
    }

    struct bufferevent *ev_watch = bufferevent_socket_new(evbase, inotifyFd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(ev_watch, config_watch_callback, NULL, NULL, NULL);
    bufferevent_enable(ev_watch, EV_READ);

    return 0;
}

//// caller

void start_watching_dir(main_ctx_t *MAIN_CTX)
{
	char path[10240] = {0,};
	sprintf(path, "%s/data", getenv("HOME"));
	watch_directory_init(MAIN_CTX->evbase_main, path, watch_sctpc_restart);
}

void watch_sctpc_restart(const char *file_name)
{           
	if (!strcmp(file_name, "sctpc_start")) {
		fprintf(stderr, "\n%s() detect \"sctpc_start\" changed!\n", __func__);
		remove_amf_list(MAIN_CTX);
		create_amf_list(MAIN_CTX);
	}
}
