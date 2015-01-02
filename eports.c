/*
 * event ports example - tcp echo service
 * echos all inputs, handles congestion, refusal to read
 * socket by clients, etc. If stdin receives a EOF,
 * port_alert is used to cause all threads to exit.
 *
 * use mconnect or telnet to port 35000 to test
 * Truss the binary to really watch whats happening...
 *
 * cc -D_REENTRANT eports.c -o eports -lsocket
 *
 *          Bart Smaalders 7/20/04
 */

#include <port.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <thread.h>

#define PORT_NUM    35000 /* arbitrary */
#define BLEN        1024

typedef struct conn {
    int         (*cn_callback)(struct conn *);
    int         cn_fd;
    int         cn_total; /* totals sent */
    int         cn_bcnt; /* bytes to be sent */
    char        cn_buffer[BLEN];
} conn_t;

static int port;

/*ARGSUSED*/
static void *
thread_loop(void *arg)
{
    port_event_t ev;
    conn_t *ptr;

    (void) printf("Thread %d starting\n", pthread_self());

    /*CONSTCOND*/
    while (1) {
        if (port_get(port, &ev, NULL) < 0) {
            perror("port_get");
            exit(1);
        }
        if (ev.portev_source == PORT_SOURCE_FD) {
            ptr = (conn_t *)ev.portev_user;
            (void) ptr->cn_callback(ptr);
        } else
            break;
    }
    (void) printf("Thread %d exiting\n", pthread_self());

    return (arg);
}

static conn_t *
get_conn()
{
    conn_t *ptr = malloc(sizeof (conn_t));

    if (!ptr) {
        perror("malloc");
        exit(1);
    }

    bzero(ptr, sizeof (*ptr));

    return (ptr);
}

static int
echo_func(conn_t *ptr)
{
    int wrote;
    int red;

    /*
     * if there's no pending data waiting to be echo'd back,
     * we must be ready to read some
     */

    if (ptr->cn_bcnt == 0) /* need to read */ {
        red = read(ptr->cn_fd, ptr->cn_buffer,
            BLEN);
        if (red <= 0) {
            (void) printf("Closing connection %d"
                " - echoed %d bytes\n",
                ptr->cn_fd, ptr->cn_total);
            (void) close(ptr->cn_fd);
            free(ptr);
            return (0);
        }
        ptr->cn_bcnt = red;
    }

    /*
     * if we have data, we need to write
     */

    if (ptr->cn_bcnt > 0) {
        wrote = write(ptr->cn_fd, ptr->cn_buffer,
            ptr->cn_bcnt);

        if (wrote > 0)
            ptr->cn_total += wrote;

        if (wrote < 0) {
            if (errno != EAGAIN) {
                (void) printf("Closing connection %d"
                    " - echoed %d bytes\n",
                    ptr->cn_fd, ptr->cn_total);
                (void) close(ptr->cn_fd);
                free(ptr);
                return (0);
            }
            wrote = 0;
        }

        if (wrote < ptr->cn_bcnt) {
            if (wrote != 0) {
                (void) memmove(ptr->cn_buffer,
                    ptr->cn_buffer + wrote,
                    ptr->cn_bcnt - wrote);
                ptr->cn_bcnt -= wrote;
            }
            /*
             * we managed to write some, but still have
             * some left. Wait for further drainage
             */

            if (port_associate(port,
                PORT_SOURCE_FD, ptr->cn_fd,
                POLLOUT, ptr) < 0) {
                perror("port_associate");
                exit(1);
            }
        } else {
            /*
             * we wrote it all
             * go back to reading
             */

            ptr->cn_bcnt = 0;
            if (port_associate(port,
                PORT_SOURCE_FD, ptr->cn_fd,
                POLLIN, ptr) < 0) {
                perror("port_associate");
                exit(1);
            }
        }
    }
    return (0);
}


static int
listen_func(conn_t *ptr)
{
    struct sockaddr_in addr;
    int alen;

    conn_t *new = get_conn();

    if ((new->cn_fd = accept(ptr->cn_fd, (struct sockaddr *)&addr,
        &alen)) < 0) {
        perror("accept");
        exit(1);
    }

    new->cn_callback = echo_func;

    /*
     * use non-blocking sockets so we don't hang threads if
     * clients are not reading their return values
     */

    if (fcntl(new->cn_fd, F_SETFL, O_NDELAY) < 0) {
        perror("fcntl");
        exit(1);
    }

    /*
     * associate new tcp connection w/ port so we can get events from it
     */


    if (port_associate(port, PORT_SOURCE_FD, new->cn_fd, POLLIN, new) < 0) {
        perror("port_associate");
        exit(1);
    }

    /*
     * re-associate listen_fd so we can accept further connections
     */

    if (port_associate(port, PORT_SOURCE_FD, ptr->cn_fd, POLLIN, ptr) <
        0) {
        perror("port_associate");
        exit(1);
    }

    (void) printf("New connection %d\n", new->cn_fd);

    return (0);
}

/*ARGSUSED*/
int
main(int argc, char *argv[])
{
    int lsock;
    int optval;
    int i;
    pthread_t tid;

    struct sockaddr_in server;
    conn_t *ptr;

    (void) sigignore(SIGPIPE);

    if ((port = port_create()) < 0) {
        perror("port_create");
        exit(1);
    }

    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket:");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT_NUM);
    optval = 1;

    if (setsockopt(lsock, SOL_SOCKET,
        SO_REUSEADDR, (char *)&optval, 4) < 0) {
        perror("setsocketopt:");
        exit(1);
    }

    if (bind(lsock, (struct sockaddr *)&server, sizeof (server)) < 0) {
        perror("bind:");
        exit(2);
    }

    (void) listen(lsock, 10);

    ptr = get_conn();
    ptr->cn_fd = lsock;
    ptr->cn_callback = listen_func;

    /*
     * associate listening socket w/ port so we can accept new cons.
     */

    if (port_associate(port, PORT_SOURCE_FD, lsock, POLLIN, ptr) < 0) {
        perror("port_associate");
        exit(1);
    }

    for (i = 0; i < 10; i++)
        (void) pthread_create(&tid, NULL, thread_loop, NULL);

    /*
     * wait for ^D on stdin
     */

    while (getchar_unlocked() != EOF)
        ;

    /*
     * set port to alert status to demo toggling port
     * causes threads to exit thread_loop
     */

    if (port_alert(port, PORT_ALERT_SET, 1, NULL) < 0) {
        perror("port_alert");
        exit(1);
    }

    /*
     * wait for all threads to exit after getting alert
     */

    while (thr_join(0, NULL, NULL) == 0)
        ;

    return (0);

}

/* vim:set ts=8 sw=4 sts=4 tw=78 et: */
