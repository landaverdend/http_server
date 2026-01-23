#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include "http_handler.h"

int http_parse_request(const char *raw, http_request_t *request)
{
    memset(request, 0, sizeof(http_request_t));

    printf("Raw request: %s\n", raw);

    // Find end of request line
    const char *line_end = strstr(raw, "\r\n");
    if (!line_end) return -1;

    // Parse request line: "GET /path HTTP/1.1"
    if (sscanf(raw, "%15s %255s %15s", request->method, request->path, request->version) != 3) {
        return -1;
    }

    // Move to headers
    const char *ptr = line_end + 2;

    // Parse headers until empty line
    while (*ptr && request->header_count < MAX_HEADERS) {
        // Empty line = end of headers
        if (ptr[0] == '\r' && ptr[1] == '\n') {
            ptr += 2;
            break;
        }

        line_end = strstr(ptr, "\r\n");
        if (!line_end) break;

        // Find colon separator
        const char *colon = strchr(ptr, ':');
        if (!colon || colon > line_end) {
            ptr = line_end + 2;
            continue;
        }

        // Copy header name
        int name_len = colon - ptr;
        if (name_len >= MAX_HEADER_NAME) name_len = MAX_HEADER_NAME - 1;
        strncpy(request->headers[request->header_count].name, ptr, name_len);
        request->headers[request->header_count].name[name_len] = '\0';

        // Skip colon and whitespace
        colon++;
        while (*colon == ' ') colon++;

        // Copy header value
        int value_len = line_end - colon;
        if (value_len >= MAX_HEADER_VALUE) value_len = MAX_HEADER_VALUE - 1;
        strncpy(request->headers[request->header_count].value, colon, value_len);
        request->headers[request->header_count].value[value_len] = '\0';

        request->header_count++;
        ptr = line_end + 2;
    }

    // Get Content-Length if present
    const char *content_len = http_get_header(request, "Content-Length");
    if (content_len) {
        request->content_length = atoi(content_len);
        if (request->content_length > 0) {
            request->body = malloc(request->content_length + 1);
            memcpy(request->body, ptr, request->content_length);
            request->body[request->content_length] = '\0';
        }
    }

    return 0;
}

const char *http_get_header(const http_request_t *request, const char *name)
{
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

char *http_build_response(http_response_t *res)
{
    // Calculate size needed
    int size = 256;  // status line + padding
    for (int i = 0; i < res->header_count; i++) {
        size += strlen(res->headers[i].name) + strlen(res->headers[i].value) + 4;
    }
    size += res->body_length + 64;

    char *response = malloc(size);
    int offset = sprintf(response, "HTTP/1.1 %d %s\r\n", res->status_code, res->status_text);

    // Add headers
    for (int i = 0; i < res->header_count; i++) {
        offset += sprintf(response + offset, "%s: %s\r\n",
            res->headers[i].name, res->headers[i].value);
    }

    // Add Content-Length if body exists
    if (res->body && res->body_length > 0) {
        offset += sprintf(response + offset, "Content-Length: %d\r\n", res->body_length);
    }

    // Empty line + body
    offset += sprintf(response + offset, "\r\n");
    if (res->body && res->body_length > 0) {
        memcpy(response + offset, res->body, res->body_length);
        offset += res->body_length;
    }
    response[offset] = '\0';

    return response;
}

void http_free_request(http_request_t *req)
{
    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
}

void http_free_response(http_response_t *res)
{
    if (res->body) {
        free(res->body);
        res->body = NULL;
    }
}

// Get MIME type from file extension
static const char *get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (strcasecmp(ext, ".html") == 0) return "text/html";
    if (strcasecmp(ext, ".css") == 0)  return "text/css";
    if (strcasecmp(ext, ".js") == 0)   return "application/javascript";
    if (strcasecmp(ext, ".json") == 0) return "application/json";
    if (strcasecmp(ext, ".png") == 0)  return "image/png";
    if (strcasecmp(ext, ".jpg") == 0)  return "image/jpeg";
    if (strcasecmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, ".gif") == 0)  return "image/gif";
    if (strcasecmp(ext, ".ico") == 0)  return "image/x-icon";
    if (strcasecmp(ext, ".txt") == 0)  return "text/plain";

    return "application/octet-stream";
}

int serve_static_file(const char *url_path, http_response_t *res)
{
    memset(res, 0, sizeof(http_response_t));

    // Security: block path traversal
    if (strstr(url_path, "..") != NULL) {
        res->status_code = 403;
        strcpy(res->status_text, "Forbidden");
        res->body = strdup("403 Forbidden");
        res->body_length = strlen(res->body);
        return -1;
    }

    // Map "/" to "/index.html"
    const char *file_path = url_path;
    if (strcmp(url_path, "/") == 0) {
        file_path = "/index.html";
    }

    // Build full path: "public" + file_path
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "public%s", file_path);

    // Open file
    FILE *f = fopen(full_path, "rb");
    if (!f) {
        res->status_code = 404;
        strcpy(res->status_text, "Not Found");
        res->body = strdup("404 Not Found");
        res->body_length = strlen(res->body);
        return -1;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read file contents
    char *contents = malloc(size);
    if (!contents) {
        fclose(f);
        res->status_code = 500;
        strcpy(res->status_text, "Internal Server Error");
        res->body = strdup("500 Internal Server Error");
        res->body_length = strlen(res->body);
        return -1;
    }

    fread(contents, 1, size, f);
    fclose(f);

    // Set response
    res->status_code = 200;
    strcpy(res->status_text, "OK");
    res->body = contents;
    res->body_length = size;

    // Set Content-Type header
    const char *mime = get_mime_type(file_path);
    strcpy(res->headers[0].name, "Content-Type");
    strcpy(res->headers[0].value, mime);
    res->header_count = 1;

    return 0;
}
