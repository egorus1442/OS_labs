#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <unistd.h>
#include <time.h>

// Error handling macro
#define FATAL(msg) { fprintf(stderr, "FATAL ERROR: %s, exiting.\n", msg); exit(EXIT_FAILURE); }
#define ERROR(msg) { fprintf(stderr, "ERROR: %s\n", msg); }

#define DEFAULT_TIMEOUT 2

// Structure to represent a computational node
typedef struct compute_node_t {
    int id;
    pid_t pid;
    int parent_id; // For topology, not always used
    void *socket; // ZeroMQ socket to this node
    struct compute_node_t *left;
    struct compute_node_t *right;
    char *last_command;
    int64_t last_command_sent;
} compute_node_t;

// Global root of the compute node tree (for topology 4)
compute_node_t *root_node = NULL;

// Function to create a new compute node structure
compute_node_t *create_compute_node(int id, int parent_id) {
    compute_node_t *node = (compute_node_t *) malloc(sizeof(compute_node_t));
    if (!node) FATAL("Failed to allocate memory for compute node");
    node->id = id;
    node->pid = 0; // PID will be set later after fork
    node->parent_id = parent_id;
    node->socket = NULL; // Socket will be created when process starts
    node->left = NULL;
    node->right = NULL;
    node->last_command = NULL;
    node->last_command_sent = -1;
    return node;
}

int count_nodes(compute_node_t *root) {
    if (root == NULL)
        return 0;

    int result = 1;
    if (root->left != NULL) {
        result += count_nodes(root->left);
    }
    if (root->right != NULL) {
        result += count_nodes(root->right);
    }
    return result;
}

compute_node_t *insert_node(compute_node_t *root, compute_node_t *new_node) {
    if (root == NULL) {
        return new_node;
    }

    if (count_nodes(root->left) <= count_nodes(root->right)) {
        root->left = insert_node(root->left, new_node);
    } else {
        root->right = insert_node(root->right, new_node);
    }
    return root;
}

// Function to find a node in the BST
compute_node_t *find_node(compute_node_t *root, int id) {
    if (root == NULL) {
        return NULL;
    }
    if (root->id == id) {
        return root;
    }

    compute_node_t *a = find_node(root->left, id);
    if (a != NULL)
        return a;

    return find_node(root->right, id);
}

// Function to send a command to a compute node and receive response
void send_command_to_node(compute_node_t *node, const char *command) {
    zmq_msg_t request;
    zmq_msg_init_size(&request, strlen(command) + 1);
    strcpy(zmq_msg_data(&request), command);

    if (zmq_msg_send(&request, node->socket, 0) == -1) {
        zmq_msg_close(&request);
        if (strcmp(command, "ping") == 0) {
            printf("Ok: 0 // узел %d недоступен\n", node->id);
        } else {
            printf("Error:%d: Node is unavailable\n", node->id);
        }
        return; // Communication error
    }
    zmq_msg_close(&request);

    node->last_command = strdup(command);
    node->last_command_sent = time(NULL);
}

void handle_response(compute_node_t *node, const char *response) {
    if (strcmp(response, "pong") == 0) {
        printf("Ok: 1 // узел %d доступен\n", node->id);
    } else if (strstr(response, "Ok") != NULL) {
        printf("Ok:%d: %s\n", node->id, &response[4]);
    } else if (strstr(response, "Error") != NULL) {
        printf("Error:%d: %s\n", node->id, &response[7]);
    } else if (strstr(node->last_command, "exec") != NULL) {
        printf("Ok:%d: %s\n", node->id, response);
    }
}

void receive_node(compute_node_t *node) {
    zmq_msg_t reply;
    zmq_msg_init(&reply);
    if (zmq_msg_recv(&reply, node->socket, ZMQ_NOBLOCK) != -1) {
        char *response = strdup((char *) zmq_msg_data(&reply));
        zmq_msg_close(&reply);
        if (node->last_command != NULL) {
            handle_response(node, response);
            node->last_command = NULL;
            node->last_command_sent = -1;
        }
        free(response);
        return;
    }
    zmq_msg_close(&reply);

    if (node->last_command_sent != -1 && time(NULL) - node->last_command_sent > DEFAULT_TIMEOUT) {
        if (strcmp(node->last_command, "ping") == 0) {
            printf("Ok: 0 // узел %d недоступен\n", node->id);
        } else {
            printf("Error:%d: Node is unavailable\n", node->id);
        }

        node->last_command = NULL;
        node->last_command_sent = -1;
    }
}

void receive_node_tree(compute_node_t *node) {
    if (node != NULL) {
        receive_node(node);
        if (node->right != NULL)
            receive_node_tree(node->right);
        if (node->left != NULL)
            receive_node_tree(node->left);
    }
}

void compute_node_process(int node_id);

int inputAvailable() {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);
    return (FD_ISSET(STDIN_FILENO, &read_fds) != 0);
}

// Controller Node logic - now reads from console
void controller_node_logic() {
    void *zmq_context = zmq_ctx_new();

    printf("Controller Node started. Listening for commands from console...\n");
    char command_buffer[256]; // Buffer for console input

    while (1) {
        receive_node_tree(root_node);

        if (!inputAvailable()) {
            usleep(100 * 1000);
            continue;
        }

        if (fgets(command_buffer, sizeof(command_buffer), stdin) == NULL) {
            if (feof(stdin)) { // Check for EOF (Ctrl+D)
                printf("Exiting on EOF.\n");
                break;
            } else {
                ERROR("Error reading from console.");
                continue;
            }
        }

        // Remove trailing newline from fgets
        command_buffer[strcspn(command_buffer, "\n")] = 0;

        if (strlen(command_buffer) == 0) { // Handle empty input
            continue;
        }
        printf("Received command: %s\n", command_buffer);

        char *command = strtok(command_buffer, " ");
        if (command != NULL) {
            if (strcmp(command, "create") == 0) {
                char *id_str = strtok(NULL, " ");
                char *parent_id_str = strtok(NULL, " "); // Ignored in Topology 4 create command

                int id = atoi(id_str);
                if (find_node(root_node, id) != NULL) {
                    printf("Error: Already exists\n");
                } else {
                    compute_node_t *new_node = create_compute_node(id, -1); // Parent ID is not used in create command for topology 4 as per instructions
                    root_node = insert_node(root_node, new_node);

                    pid_t pid = fork();
                    if (pid == 0) {
                        // Child process (Compute Node)
                        compute_node_process(id);
                        exit(EXIT_SUCCESS); // Child process exits after compute node logic
                    } else if (pid > 0) {
                        new_node->pid = pid;
                        // Wait for compute node to start and connect (very basic sync, better approach would be needed for robust system)
                        sleep(1); // Give time for node to start and bind. In real system, use proper handshake.
                        char address[50];
                        sprintf(address, "tcp://127.0.0.1:%d", 5550 + id); // Assuming port allocation based on ID
                        new_node->socket = zmq_socket(zmq_context, ZMQ_REQ);
                        int connect_result = zmq_connect(new_node->socket, address);
                        if (connect_result != 0) {
                            ERROR("Failed to connect to compute node, cleanup needed.");
                            // TODO: Handle cleanup if connection fails, remove node from tree, kill process if needed.
                            printf("Error: Parent is unavailable\n"); // Best guess for this lab based on error messages.
                        }
                        printf("Ok: %d\n", new_node->pid);
                    } else {
                        ERROR("Fork failed");
                        // Handle fork error, maybe remove node from tree if partially created.
                        printf("Error: [System error]\n"); // Generic error for fork failure.
                    }
                }
            } else if (strcmp(command, "exec") == 0) {
                char *id_str = strtok(NULL, " ");
                int id = atoi(id_str);
                compute_node_t *node = find_node(root_node, id);
                if (node == NULL) {
                    printf("Error:%d: Not found\n", id);
                } else {
                    char *params = strtok(NULL, ""); // Get the rest of the string as parameters
                    char exec_command[200];
                    sprintf(exec_command, "exec %s", params);

                    send_command_to_node(node, exec_command);
                }
            } else if (strcmp(command, "ping") == 0) {
                char *id_str = strtok(NULL, " ");

                int id = atoi(id_str);
                compute_node_t *node = find_node(root_node, id);
                if (node == NULL) {
                    printf("Error: Not found\n");
                } else {
                    send_command_to_node(node, "ping");
                }
            } else {
                printf("Error: [Unknown command]\n");
            }
        }
    }

    zmq_ctx_destroy(zmq_context);
}


// Compute Node process logic (same as before)
void compute_node_process(int node_id) {
    void *zmq_context = zmq_ctx_new();
    void *responder = zmq_socket(zmq_context, ZMQ_REP);
    char address[50];
    sprintf(address, "tcp://127.0.0.1:%d", 5550 + node_id); // Bind to port based on ID
    int bind_result = zmq_bind(responder, address);
    if (bind_result != 0) {
        printf("Error code: %d", bind_result);
        FATAL("Compute node bind failed");
    }
    printf("Compute Node %d started, listening on %s\n", node_id, address);

    while (1) {
        zmq_msg_t request;
        zmq_msg_init(&request);
        zmq_msg_recv(&request, responder, 0);
        char *command_str = strdup((char *) zmq_msg_data(&request));
        zmq_msg_close(&request);

        char *response_str = NULL;
        char *command = strtok(command_str, " ");

        if (strcmp(command, "exec") == 0) {
            char *params_str = strtok(NULL, ""); // Get all parameters after "exec"

            char *n_str = strtok(params_str, " ");

            int n = atoi(n_str);
            int sum = 0;
            for (int i = 0; i < n; ++i) {
                char *k_str = strtok(NULL, " ");
                sum += atoi(k_str);
            }

            char buffer[50];
            int written = sprintf(buffer, "%d", sum);
            buffer[written] = '\0';
            response_str = strdup(buffer);
        } else if (strcmp(command, "ping") == 0) {
            response_str = strdup("pong");
        } else {
            response_str = strdup("Error: Unknown command for compute node");
        }


        zmq_msg_t reply;
        zmq_msg_init_size(&reply, strlen(response_str) + 1);
        strcpy(zmq_msg_data(&reply), response_str);
        free(response_str);

        zmq_msg_send(&reply, responder, 0);
        zmq_msg_close(&reply);
        free(command_str);
    }
    zmq_close(responder);
    zmq_ctx_destroy(zmq_context);
}


int main() {
    // Start controller logic, now reading from console directly
    controller_node_logic();

    return 0;
}