/* request.c: HTTP Request Functions */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

int parse_request_method(struct request *r);
int parse_request_headers(struct request *r);

/**
 * Accept request from server socket.
 *
 * This function does the following:
 *
 *  1. Allocates a request struct initialized to 0.
 *  2. Initializes the headers list in the request struct.
 *  3. Accepts a client connection from the server socket.
 *  4. Looks up the client information and stores it in the request struct.
 *  5. Opens the client socket stream for the request struct.
 *  6. Returns the request struct.
 *
 * The returned request struct must be deallocated using free_request.
 **/
struct request *
accept_request(int sockfd)
{
    struct request *req;
    struct sockaddr raddr;
    socklen_t rlen;
    size_t reqn = 1;

    /* Allocate request struct (zeroed) */
    req = (struct request *)calloc(reqn, sizeof(struct request));
    if (req == NULL)
    {
        fprintf(stderr, "accept_request(): Memory allocation failed\n");
        return NULL;
    }
    /* Accept a client */
    if ((req->fd = accept(sockfd, &raddr, &rlen)) < 0)
    {
        fprintf(stderr, "Failed to accept request");
        goto fail;
    }
    /* Lookup client information */
    if (getnameinfo(&raddr, rlen, req->host, sizeof(req->host), req->port, sizeof(req->port), 0) != 0)
    {
        fprintf(stderr, "Host name and service cannot be retrieved");
        goto fail;
    }
    /* Open socket stream */
    if ((req->file = fdopen(req->fd, "r")) == NULL)
    {
        fprintf(stderr, "Cannot open client socket stream");
        goto fail;
    }
    log("Accepted request from %s:%s", req->host, req->port);
    return req;

fail:
    free_request(req);
    return NULL;
}

/**
 * Deallocate request struct.
 *
 * This function does the following:
 *
 *  1. Closes the request socket stream or file descriptor.
 *  2. Frees all allocated strings in request struct.
 *  3. Frees all of the headers (including any allocated fields).
 *  4. Frees request struct.
 **/
void free_request(struct request *req)
{
    struct header *header;

    if (req == NULL)
    {
        return;
    }

    /* Close socket or fd */
    close(req->fd);
    /* Free allocated strings */
    free(req->method);
    free(req->query);
    free(req->path);
    free(req->uri);
    /* Free headers */
    header = req->headers;
    while (header != NULL)
    {
        struct header *nextHeader = header->next;
        free(header->name);
        free(header->value);
        free(header);
        header = nextHeader;
    }
    /* Free request */
    free(req);
}

/**
 * Parse HTTP Request.
 *
 * This function first parses the request method, any query, and then the
 * headers, returning 0 on success, and -1 on error.
 **/
int parse_request(struct request *req)
{
    /* Parse HTTP Request Method */
    if (parse_request_method(req) < 0)
    {
        fprintf(stderr, "parse_request: Failed to parse request method");
        return -1;
    };
    /* Parse HTTP Requet Headers*/
    if (parse_request_headers(req) < 0)
    {
        fprintf(stderr, "parse_request: Failed to parse request headers");
        return -1;
    }
    return 0;
}

/**
 * Parse HTTP Request Method and URI
 *
 * HTTP Requests come in the form
 *
 *  <METHOD> <URI>[QUERY] HTTP/<VERSION>
 *
 * Examples:
 *
 *  GET / HTTP/1.1
 *  GET /cgi.script?q=foo HTTP/1.0
 *
 * This function extracts the method, uri, and query (if it exists).
 **/
int parse_request_method(struct request *r)
{
    /* Read line from socket */
    char *buff = NULL;
    size_t ln;
    if (getline(&buff, &ln, r->file) < 0)
    {
        fprintf(stderr, "Failed to parse request method");
        return -1;
    }

    /* Parse method and uri */
    char *method_begin;
    char *method;
    char *whitespace_begin;
    char *uri_begin;
    char *uri_and_query;
    char *uri;
    char *query;

    if ((method_begin = skip_whitespace(buff)) == NULL)
    {
        fprintf(stderr, "skip_whitespace: Failed to parse request method.\n");
        goto fail;
    }
    if ((method = strtok(method_begin, WHITESPACE)) == NULL)
    {
        fprintf(stderr, "strtok: Failed to parse request method.\n");
        goto fail;
    }
    if ((whitespace_begin = skip_nonwhitespace(method_begin)) == NULL)
    {
        fprintf(stderr, "skip_nonwhitespace: Failed to parse request uri.\n");
        goto fail;
    }
    if ((uri_begin = skip_whitespace(whitespace_begin) == NULL))
    {
        fprintf(stderr, "skip_whitespace: Failed to parse request uri\n");
        goto fail;
    }
    if ((uri_and_query = strtok(uri_begin, WHITESPACE)) == NULL)
    {
        fprintf(stderr, "strtok: Failed to parse uri and query\n");
        goto fail;
    }
    if ((uri = strtok(uri_and_query, "?")) == NULL)
    {
        fprintf(stderr, "strtok: Failed to parse uri.\n");
        goto fail;
    }
    if ((query = strchr(uri_and_query, "?") + 1) == NULL)
    {
        fprintf(stderr, "strch: Failed to parse querry.\n");
        goto fail;
    }
    /* Record method, uri, and query in request struct */
    if (
        (r->method = strdup(method)) == NULL ||
        (r->uri = strdup(uri)) == NULL ||
        (r->query = strdup(query)) == NULL)
    {
        fprintf(stderr, "Memory allocation failed for request attributes\n");
        goto fail;
    }

    debug("HTTP METHOD: %s", r->method);
    debug("HTTP URI:    %s", r->uri);
    debug("HTTP QUERY:  %s", r->query);

    return 0;

fail:
    return -1;
}

/**
 * Parse HTTP Request Headers
 *
 * HTTP Headers come in the form:
 *
 *  <NAME>: <VALUE>
 *
 * Example:
 *
 *  Host: localhost:8888
 *  User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:29.0) Gecko/20100101 Firefox/29.0
 *  Accept: text/html,application/xhtml+xml
 *  Accept-Language: en-US,en;q=0.5
 *  Accept-Encoding: gzip, deflate
 *  Connection: keep-alive
 *
 * This function parses the stream from the request socket using the following
 * pseudo-code:
 *
 *  while (buffer = read_from_socket() and buffer is not empty):
 *      name, value = buffer.split(':')
 *      header      = new Header(name, value)
 *      headers.append(header)
 **/
int parse_request_headers(struct request *r)
{
    struct header *curr = NULL;
    char buffer[BUFSIZ];
    char *name;
    char *value;

    /* Parse headers from socket */

#ifndef NDEBUG
    for (struct header *header = r->headers; header != NULL; header = header->next)
    {
        debug("HTTP HEADER %s = %s", header->name, header->value);
    }
#endif
    return 0;

fail:
    return -1;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
