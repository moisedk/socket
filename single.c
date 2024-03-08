/* single.c: Single User HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

/**
 * Handle one HTTP request at a time
 **/
void
single_server(int sfd)
{
    struct request *request;

    /* Accept and handle HTTP request */
    while (true) {
    	/* Accept request */

	/* Handle request */

	/* Free request */
    }

    /* Close socket and exit */
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
