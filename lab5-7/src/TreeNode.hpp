#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <string>
#include <future>
#include <vector>
#include <sstream>
#include <thread>
#include <unordered_map>

#include "SocketCommunication.hpp"
#include "Utils.hpp"

class TreeNode {
private:
    int id;
    std::optional<zmq::socket_t> parent_socket;
    std::optional<zmq::socket_t> left_socket;
    std::optional<zmq::socket_t> right_socket;

    zmq::context_t context;

    zmq::pollitem_t ps;
    zmq::pollitem_t ls;
    zmq::pollitem_t rs;

    bool running;

private:
    void process_message(const std::string& message) {
        if (message == "recursive_destroy") {
            running = false;
            if (left_socket.has_value()) {
                Send(left_socket.value(), "recursive_destroy");
                left_socket = std::nullopt;
            }
            if (right_socket.has_value()) {
                Send(right_socket.value(), "recursive_destroy");
                right_socket = std::nullopt;
            }

            return;
        }
        std::vector<std::string> tokens = split(message, ';');
        std::string request_id = tokens[0];
        std::string cmd = tokens[1];
        std::string path = tokens[2];
        std::string id = tokens[3];

        std::string unavailable_message;
        if (cmd == "ping") {
            unavailable_message = "Ok: 0";
        } else if (cmd == "exec") {
            unavailable_message = "Error:" + id + ": Node is unavailable";
        } else if (cmd == "bind") {
            unavailable_message = "Error";
        }

        if (path.size() != 0) {
            tokens[2] = path.substr(1, path.size() - 1);

            if (path[0] == 'l' and left_socket.has_value()) {
                if (!Send(left_socket.value(), join(tokens, ";"))) {
                    Send(parent_socket.value(), join({request_id, unavailable_message}, ";"));
                }
            } else if (path[0] == 'r' and right_socket.has_value()) {
                if (!Send(right_socket.value(), join(tokens, ";"))) {
                    Send(parent_socket.value(), join({request_id, unavailable_message}, ";"));
                }
            } else {
                Send(parent_socket.value(), join({request_id, "Error: Not found"}, ";"));
            }

            return;
        }

        if (cmd == "ping") {
            Send(parent_socket.value(), join({request_id, "Ok: 1"}, ";"));
        } else if (cmd == "bind") {
            std::string side = tokens[4];
            std::string port = tokens[5];
            if (side == "left") {
                setup_left_socket(port);
            } else if (side == "right") {
                setup_right_socket(port);
            }
            Send(parent_socket.value(), join({request_id, "Ok: Binded"}, ";"));
        } else if (cmd == "exec") {
            std::stringstream iss(tokens[4]);
            int n;
            iss >> n;
            int sum = 0;
            for (int i = 0; i < n; ++i) {
                int cur;
                iss >> cur;
                sum += cur;
            }
            std::string message = "Ok:" + id + ": " + std::to_string(sum);
            Send(parent_socket.value(), join({request_id, message}, ";"));
        } else {
            Send(parent_socket.value(), join({request_id, "Error: Unknown command"}, ";"));
        }
    }

public:
    TreeNode(int node_id) : id(node_id), context(1), running(true) {}

    void setup_parent_socket(const std::string& port) {
        if (!parent_socket.has_value()) {
            parent_socket.emplace(context, ZMQ_PAIR);
            parent_socket->connect("tcp://localhost:" + port);

            // Устанавливаем параметры сокета
            parent_socket->set(zmq::sockopt::linger, 2000);   // Удаление сообщений, если отправить нельзя
            parent_socket->set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
            parent_socket->set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку

            ps = zmq::pollitem_t{parent_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Parent socket connected to tcp://*:" << port << std::endl;
        }
    }

    void setup_left_socket(const std::string& port) {
        if (!left_socket.has_value()) {
            left_socket.emplace(context, ZMQ_PAIR);
            left_socket->bind("tcp://*:" + port);

            // Устанавливаем параметры сокета
            left_socket->set(zmq::sockopt::linger, 2000);   // Удаление сообщений, если отправить нельзя
            left_socket->set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
            left_socket->set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку
    
            ls = zmq::pollitem_t{left_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Left socket bound to tcp://localhost:" << port << std::endl;
        }
    }

    void setup_right_socket(const std::string& port) {
        if (!right_socket.has_value()) {
            right_socket.emplace(context, ZMQ_PAIR);
            right_socket->bind("tcp://*:" + port);

            // Устанавливаем параметры сокета
            right_socket->set(zmq::sockopt::linger, 2000);   // Удаление сообщений, если отправить нельзя
            right_socket->set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
            right_socket->set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку
    
            rs = zmq::pollitem_t{right_socket.value(), 0, ZMQ_POLLIN, 0};
            std::cout << "Right socket bound to tcp://localhost:" << port << std::endl;
        }
    }

    void run() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // Обработка сообщений от родителя
            if (parent_socket.has_value()) {
                zmq::pollitem_t items[] = {ps};
                zmq::poll(items, 1, std::chrono::milliseconds(0));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(parent_socket.value());
                    if (message.has_value()) {
                        // std::cout << "[COMP_NODE] Processing " << message.value() << std::endl;
                        process_message(message.value());
                    }
                }
            }

            // Пересылаем сообщения от левого дочернего узла к родителю
            if (left_socket.has_value()) {
                zmq::pollitem_t items[] = {ls};
                zmq::poll(items, 1, std::chrono::milliseconds(0));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(left_socket.value());
                    if (message.has_value()) {
                        // std::cout << "[COMP_NODE] Resend " << message.value() << std::endl;
                        Send(parent_socket.value(), message.value());
                    }
                }
            }

            // Пересылаем сообщения от правого дочернего узла к родителю
            if (right_socket.has_value()) {
                zmq::pollitem_t items[] = {rs};
                zmq::poll(items, 1, std::chrono::milliseconds(0));

                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(right_socket.value());
                    if (message.has_value()) {
                        // std::cout << "[COMP_NODE] Resend " << message.value() << std::endl;
                        Send(parent_socket.value(), message.value());
                    }
                }
            }
        }
    }


    ~TreeNode() {
        std::cout << "Destroying Node " << id << std::endl;

        try {
            if (left_socket.has_value() && left_socket->handle() != nullptr) {
                Send(left_socket.value(), "recursive_destroy");
                left_socket->close(); // Закрытие сокета
            }

            if (right_socket.has_value() && right_socket->handle() != nullptr) {
                Send(right_socket.value(), "recursive_destroy");
                right_socket->close(); // Закрытие сокета
            }

            if (parent_socket.has_value() && parent_socket->handle() != nullptr) {
                parent_socket->close(); // Закрытие родительского сокета
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "Error while destroying TreeNode: " << e.what() << std::endl;
        }

        context.close(); // Закрытие контекста
        std::cout << "Node " << id << " destroyed." << std::endl;
    }
};
