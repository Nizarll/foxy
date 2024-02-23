#include "libs/utils.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
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
  router_tree.root = bin_t_insert(router_tree.root, "/about",
                                  &(struct Route){
                                      .html_path = "route/about/about.html",
                                      .css_path = "route/about/about.css",
                                  });
  router_tree.root =
      bin_t_insert(router_tree.root, "/projects",
                   &(struct Route){
                       .html_path = "route/projects/projects.html",
                       .css_path = "route/projects/projects.css",
                   });
}

void handle_client(void **network_nodes) {
  // get the client struct ( encapsulation of the client socket ) from the void*
  // args given from pthread_t
  warn("%d %d", ((struct Client *)(network_nodes[1]))->sockfd,
       ((struct Server *)(network_nodes[1]))->sockfd);
  struct Client client = *((struct Client *)(((void **)network_nodes)[0]));
  okay("client ip : %s connected to the server",
       get_client_addr_str(((void **)network_nodes)[0]));
  // handle client runs whenever a client connects to the server socket
  char *html_file;
  JSON_FORMAT(router_tree.root->route->html_path, html_file);
  warn("%ld length of file ", strlen(html_file));
  char buffer[50], response[strlen(html_file) + 100];
  bzero(&buffer, 50);
  read(client.sockfd, buffer, 50);
  handle_http_request(buffer);
  sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  strcat(response, html_file);
  strcat(response, "\r\n");
  int bytes_sent = send((client.sockfd), &response, strlen(response), 0);
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
