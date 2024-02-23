#include "utils.h"
#include <netinet/in.h>

struct Bintree router_tree = (struct Bintree){
    .root =
        &(struct Node){
            .key = "/index",
            .route =
                &(struct Route){
                    .html_path = "route/index.html",
                    .css_path = "route/index.css",
                },
        },
};

char *handle_http_request(char *http_request) {
  okay("%s", http_request);
  char *token = strtok(http_request, " \n");
  char *file = NULL;
  char *extension = NULL;
  while (token != NULL) {
    if (strcmp(token, "GET") == 0) {
      token = strtok(NULL, " \n");
      if (token != NULL) {
        file = strtok(token, ".");
        extension = strtok(NULL, ".");
      }
      break; // Exit the loop after finding "GET" method
    }
    token = strtok(NULL, " \n");
  }

  // If file and extension are found, print them
  if (file != NULL && extension != NULL) {
    printf("Requested file: %s\n", file);
    printf("Extension: %s\n", extension);
  } else {
    printf("%s %s", extension, file);
    printf("Invalid HTTP request or missing file extension.\n");
  }
  okay("%s, %s", file, extension);
  return file; // Return the file name (may be NULL)
}

struct Server server_init(int backlog, int domain, int service, int protocol,
                          int port, char *address,
                          void (*handle_client)(void **)) {
  struct Server server;
  memset(&server, 0, sizeof(struct Server));
  server.backlog = backlog;
  server.domain = domain;
  server.service = service;
  server.protocol = protocol;
  server.port = port;
  server.address.sin_family = domain;
  server.address.sin_port = htons(port);
  server.address.sin_addr.s_addr = inet_addr("127.0.0.1");

  server.sockfd = socket(server.domain, server.service, server.protocol);

  if (server.sockfd == 0)
    err("Failed to connect to socket %s", strerror(errno));
  okay("Created Server Successfully ! ");
  if ((bind(server.sockfd, (struct sockaddr *)&server.address,
            sizeof(server.address))) < 0)
    err("failed to bind socket %s", strerror(errno));
  if ((listen(server.sockfd, server.backlog)) < 0)
    err("failed to listen to the socket %s", strerror(errno));

  server.handle_client = handle_client;
  return server;
}

struct Client client_init(struct Server *server) {
  struct Client client;
  client.socklen = sizeof(struct sockaddr_in);
  if ((client.sockfd = accept(server->sockfd, (struct sockaddr *)&client.addr,
                              (socklen_t *)&client.socklen)) < 0) {
    warn("client failed to connect to server %s", strerror(errno));
  }
  return client;
}

char *get_client_addr_str(struct Client *client) {
  char *str = malloc(INET_ADDRSTRLEN +
                     6); // Maximum size for IPv4 + port (xxx.xxx.xxx.xxx:ppppp)

  if (str == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  char ip[INET_ADDRSTRLEN];
  const char *ip_str = inet_ntop(AF_INET, &(client->addr), ip, INET_ADDRSTRLEN);
  if (ip_str == NULL) {
    perror("inet_ntop");
    free(str);
    exit(EXIT_FAILURE);
  }

  snprintf(str, INET_ADDRSTRLEN + 6, "%s:%d", ip, 3000);

  return str;
}

struct Node *bin_t_create_node(char *key, struct Route *route) {
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  if (newNode == NULL) {
    printf("Memory allocation failed\n");
    exit(1);
  }
  newNode->key = strdup(key);
  newNode->route = route;
  newNode->left = newNode->right = NULL;
  return newNode;
}

struct Node *bin_t_insert(struct Node *root, char *key, struct Route *route) {
  if (root == NULL) {
    return bin_t_create_node(key, route);
  }
  int cmp = strcmp(key, root->key);
  if (cmp < 0) {
    root->left = bin_t_insert(root->left, key, route);
  } else if (cmp > 0) {
    root->right = bin_t_insert(root->right, key, route);
  } else {
    warn("binary tree key already has a value");
  }
  return root;
}

struct Node *bin_t_lookup(struct Node *root, char *key) {
  if (root == NULL || strcmp(root->key, key) == 0) {
    return root;
  }
  if (strcmp(key, root->key) < 0) {
    return bin_t_lookup(root->left, key);
  } else {
    return bin_t_lookup(root->right, key);
  }
}

void bin_t_free(struct Node *root) {
  if (root != NULL) {
    bin_t_free(root->left);
    bin_t_free(root->right);
    free(root->key);
    free(root);
  }
}
