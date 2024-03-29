#include "utils.h"
#include <netinet/in.h>

#define HEADER_LEN 4096

struct Bintree *router_tree = &(struct Bintree){
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
  char *token = strtok(http_request, " \n");
  char *filename = NULL, *extension = NULL, *content = NULL;
  char header[HEADER_LEN] = "HTTP/1.1 200 OK\r\nContent-Type: ";
  while (token != NULL) {
    if (strcmp(token, "GET") == 0) {
      token = strtok(NULL, " \n");
      if (token != NULL) {
        filename = strtok(token, ".");
        extension = strtok(NULL, ".");
      }
      break;
    }
    token = strtok(NULL, " \n");
  }
  printf("INFO: %s %s\n", filename, extension);
  if (filename != NULL) {
    content = NULL;
    if (extension == NULL) {
      strcat(header, "text/html\r\n\r\n");
      if (strcmp(filename, "/") == 0) {
        JSON_FORMAT(router_tree->root->route->html_path, content);
      } else {
        struct Node *node = bin_t_lookup(router_tree->root, filename);
        if (node != NULL) {
          okay("ight");
          printf("\nINFO: %s %s", node->key, node->route->css_path);
          printf("\n route is %s", node->route->html_path);
          JSON_FORMAT(node->route->html_path, content);
        } else {
          warn("could not find route");
        }
      }
    } else {
      if (strcmp(extension, "css") == 0) {
        strcat(header, "text/css\r\n\r\n");
        struct Node *node = bin_t_lookup(router_tree->root, filename);
        if (node != NULL) {
          JSON_FORMAT(node->route->css_path, content);
        } else {
          warn("could not find file");
        }
      } else {
        warn("unsupported file extension");
      }
    }
    if (content == NULL) {
      warn("failed to load content");
      return NULL;
    }
    char *response = malloc(strlen(header) + strlen(content) + 5);
    if (response == NULL) {
      warn("failed to allocate memory for response");
      return NULL;
    }
    strcpy(response, header);
    strcat(response, content);
    strcat(response, "\r\n");
    return response;
  }
  return NULL;
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

struct Node *bin_t_create_node(char *key, struct Route route) {
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  if (newNode == NULL) {
    printf("Memory allocation failed\n");
    exit(1);
  }
  struct Route *new_route = malloc(sizeof(struct Route));

  if (route.html_path)
    new_route->html_path = strdup(route.html_path);
  if (route.css_path)
    new_route->css_path = strdup(route.css_path);
  if (route.js_path)
    new_route->js_path = strdup(route.js_path);

  newNode->key = strdup(key);
  newNode->route = new_route;
  newNode->left = newNode->right = NULL;
  return newNode;
}

struct Node *bin_t_insert(struct Node *root, struct Node *node) {
  if (root == NULL) {
    return node;
  }

  // Compare the key of the new node with the key of the root node
  int cmp = strcmp(node->key, root->key);

  // If the key of the new node is less than the key of the root node, insert
  // into the left subtree
  if (cmp < 0) {
    root->left = bin_t_insert(root->left, node);
  }
  // If the key of the new node is greater than the key of the root node, insert
  // into the right subtree
  else if (cmp > 0) {
    root->right = bin_t_insert(root->right, node);
  }
  // If the keys are equal, update the route information
  else {
    root->route = node->route;
  }
  // Return the root node after insertion
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
