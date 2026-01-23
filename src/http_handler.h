
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#define MAX_HEADERS 32
#define MAX_HEADER_NAME 64
#define MAX_HEADER_VALUE 256

typedef struct
{
  char name[MAX_HEADER_NAME];
  char value[MAX_HEADER_VALUE];
} http_header_t;

typedef struct
{
  char method[16];
  char path[256];
  char version[16];
  http_header_t headers[MAX_HEADERS];
  int header_count;
  char *body;
  int content_length;
} http_request_t;

typedef struct
{
  int status_code;
  char status_text[64];
  http_header_t headers[MAX_HEADERS];
  int header_count;
  char *body;
  int body_length;
} http_response_t;

// Parse raw request buffer into http_request_t struct
// return 0 on success, -1 on error
int http_parse_request(const char *raw, http_request_t *request);

// Get header value by name
// return NULL if header not found
const char *http_get_header(const http_request_t *request, const char *name);

// build response string from http_response_t struct
// return malloc'd string, free after use
char *http_build_response(http_response_t *res);

// free memory allocated for http_request_t struct
void http_free_request(http_request_t *req);

// free memory allocated for http_response_t struct
void http_free_response(http_response_t *res);

// serve a static file from the public directory
// returns 0 on success, -1 on error (sets appropriate status code)
int serve_static_file(const char *path, http_response_t *res);

#endif
