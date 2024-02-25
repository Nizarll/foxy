#include "libs/utils.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_NAME 100
#define MAX_TYPE_LEN 10
#define MAX_TEXT_LEN 1024
#define MAX_CHILDREN 100

#define JSON_FORMAT(FILE_NAME, HOLDER)                                         \
  do {                                                                         \
    FILE *file;                                                                \
    uint64_t length = 0;                                                       \
    file = fopen(FILE_NAME, "r");                                              \
    if (file == NULL) {                                                        \
      err("could not read file");                                              \
      break;                                                                   \
    }                                                                          \
    if (fseek(file, 0, SEEK_END) != 0) {                                       \
      err("could not seek end of file");                                       \
      fclose(file);                                                            \
      break;                                                                   \
    }                                                                          \
    length = ftell(file);                                                      \
    if (length == -1) {                                                        \
      err("could not determine file length");                                  \
      fclose(file);                                                            \
      break;                                                                   \
    }                                                                          \
    if (fseek(file, 0, SEEK_SET) != 0) {                                       \
      err("could not seek set");                                               \
      fclose(file);                                                            \
      break;                                                                   \
    }                                                                          \
    HOLDER = malloc(length);                                                   \
    if (!HOLDER) {                                                             \
      err("could not allocate memory to the file string buffer");              \
      fclose(file);                                                            \
      break;                                                                   \
    }                                                                          \
    if (fread(HOLDER, 1, length, file) != length) {                            \
      err("error reading file content");                                       \
      free(HOLDER);                                                            \
      fclose(file);                                                            \
      break;                                                                   \
    }                                                                          \
    fclose(file);                                                              \
  } while (0)

void setup_router() {
  router_tree->root = bin_t_insert(
      router_tree->root,
      bin_t_create_node("/projects", (struct Route){
                                         .html_path = "route/about/about.html",
                                         .css_path = "route/about/about.css",
                                     }));
  router_tree->root = bin_t_insert(
      router_tree->root,
      bin_t_create_node("/about",
                        (struct Route){.html_path = "route/about/about.html",
                                       .css_path = "route/index.css"}));
}

void handle_client(void **network_nodes) {
  // get the client struct ( encapsulation of the client socket ) from the
  // void* args given from pthread_t
  struct Client client = *((struct Client *)(((void **)network_nodes)[0]));
  char request[4096];
  bzero(&request, 4096);
  read(client.sockfd, request, 4095);
  char *http_response = handle_http_request(request);
  if (http_response == NULL) {
    close(client.sockfd);
    return;
  }
  int bytes_sent =
      send((client.sockfd), http_response, strlen(http_response), 0);
  free(http_response);
  if (bytes_sent < 0) {
    err("could not send data %s", strerror(errno));
  }
  close(client.sockfd);
}

int main(void) {
  struct Server server = server_init(100, AF_INET, SOCK_STREAM, 0, 3000,
                                     "127.0.0.1", handle_client);
  setup_router();
  for (;;) {
    struct Client client = client_init(&server);
    void *network_nodes[2];
    network_nodes[0] = &client;
    network_nodes[1] = &server;
    if (client.sockfd < 0) {
      continue;
    } else {
      pthread_t thread_id;
      pthread_create(&thread_id, NULL, (void *(*)(void *))handle_client,
                     (void **)network_nodes);
      pthread_detach(thread_id);
    }
  }
  return EXIT_SUCCESS;
}
