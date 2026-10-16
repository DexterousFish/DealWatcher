dhflgdshlthkdshkthklsdjrashtsdhktsaj/*
 * ebay_search.c - search active eBay listings with the Browse API.
 *
 * Build:  gcc ebay_search.c -o ebay_search -lcurl
 * Run:    export EBAY_CLIENT_ID="your App ID"
 *         export EBAY_CLIENT_SECRET="your Cert ID"
 *         ./ebay_search "super mario world snes"
 *
 * Use api.sandbox.ebay.com + sandbox keys for testing, or
 * api.ebay.com + production keys for real data.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define API_HOST "https://api.ebay.com"   /* sandbox: https://api.sandbox.ebay.com */

/* A growable buffer that holds the server's response. */
struct Buffer {
    char  *data;
    size_t size;
};

/* libcurl calls this every time a chunk of the response arrives. */
static size_t write_cb(void *chunk, size_t size, size_t nmemb, void *userp)
{
    size_t n = size * nmemb;
    struct Buffer *buf = userp;

    char *p = realloc(buf->data, buf->size + n + 1);
    if (!p) return 0;                     /* out of memory: abort transfer */
    buf->data = p;
    memcpy(buf->data + buf->size, chunk, n);
    buf->size += n;
    buf->data[buf->size] = '\0';
    return n;
}

/* Step 1: trade client ID + secret for a temporary access token. */
static char *get_token(const char *id, const char *secret)
{
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct Buffer buf = { NULL, 0 };
    char userpwd[1024];
    snprintf(userpwd, sizeof userpwd, "%s:%s", id, secret);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers,
              "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, API_HOST "/identity/v1/oauth2/token");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
        "grant_type=client_credentials"
        "&scope=https%3A%2F%2Fapi.ebay.com%2Foauth%2Fapi_scope");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK || !buf.data) {
        fprintf(stderr, "Token request failed: %s\n", curl_easy_strerror(rc));
        free(buf.data);
        return NULL;
    }

    /* Crude JSON "parsing": find "access_token":"....". Fine for learning;
       use a real JSON library (cJSON) once you need more fields. */
    const char *key = "\"access_token\":\"";
    char *start = strstr(buf.data, key);
    if (!start) {
        fprintf(stderr, "No token in response:\n%s\n", buf.data);
        free(buf.data);
        return NULL;
    }
    start += strlen(key);
    char *end = strchr(start, '"');
    if (!end) { free(buf.data); return NULL; }

    char *token = strndup(start, (size_t)(end - start));
    free(buf.data);
    return token;
}

/* Step 2: use the token to search active listings. */
static int search_items(const char *token, const char *query, int limit)
{
    CURL *curl = curl_easy_init();
    if (!curl) return 1;

    char *escaped = curl_easy_escape(curl, query, 0);   /* "a b" -> "a%20b" */
    char url[2048];
    snprintf(url, sizeof url,
             API_HOST "/buy/browse/v1/item_summary/search?q=%s&limit=%d",
             escaped, limit);
    curl_free(escaped);

    char auth[4096];
    snprintf(auth, sizeof auth, "Authorization: Bearer %s", token);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth);
    headers = curl_slist_append(headers, "X-EBAY-C-MARKETPLACE-ID: EBAY_US");

    struct Buffer buf = { NULL, 0 };
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (rc != CURLE_OK) {
        fprintf(stderr, "Search failed: %s\n", curl_easy_strerror(rc));
    } else {
        printf("HTTP %ld\n%s\n", status, buf.data ? buf.data : "");
    }

    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK ? 0 : 1;
}

int main(int argc, char **argv)
{
    const char *id     = getenv("EBAY_CLIENT_ID");
    const char *secret = getenv("EBAY_CLIENT_SECRET");
    if (!id || !secret || argc < 2) {
        fprintf(stderr,
            "Usage: EBAY_CLIENT_ID=... EBAY_CLIENT_SECRET=... %s \"search terms\"\n",
            argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    char *token = get_token(id, secret);
    int result = 1;
    if (token) {
        result = search_items(token, argv[1], 5);
        free(token);
    }

    curl_global_cleanup();
    return result;
}tuo

thsal;thasdhtklasdhktj