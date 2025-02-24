#pragma once

#include <zmq.hpp>
#include <optional>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <future>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "SocketCommunication.hpp"
#include "SusBinHeap.hpp"
#include "Utils.hpp"

class ControlNode {
private:
    // Для работы с топологией
    SusBinHeap topology;

    // Для общения с корневым вычислительным узлом
    zmq::context_t context;
    std::string port;
    zmq::socket_t socket;

    // Для асинхронной обработки
    std::unordered_map<int, std::promise<std::string>> request_promise_map;
    std::vector<std::future<void>> futures;

    bool running;  // Для включения/выключения

private:
    std::string request_helper(const std::string& id, const std::string& cmd, const std::vector<std::string>& params) {
        if (!topology.find(std::stoi(id))) {
            return "Error: Not found";
        }
        int req_id = generate_request_id();
        std::string path = topology.get_path_to(std::stoi(id));

        std::vector<std::string> all_params = {std::to_string(req_id), cmd, path, id};
        all_params.insert(all_params.end(), params.begin(), params.end());
        std::string message = join(all_params, ";");

        std::promise<std::string> promise;
        std::future<std::string> future = promise.get_future();
        request_promise_map[req_id] = std::move(promise);

        if (!Send(socket, message)) {
            return "Error: Failed " + cmd;
        }

        return future.get();
    }

    std::string ping(const std::string& id) {
        return request_helper(id, "ping", {});
    }

    std::string bind(int parent_id, const std::string& side, const std::string& bind_port) {
        std::string ping_response = ping(std::to_string(parent_id));
        if (ping_response != "Ok: 1") {
            return "Error: Node is unavailable";
        }

        return request_helper(std::to_string(parent_id), "bind", {side, bind_port});
    }

    std::string create(const std::string& id) {
        // Проверяем, существует ли узел с таким айди
        int iid = std::stoi(id);
        if (topology.find(iid)) {
            return "Error: Already exists";
        }

        // Добавляем узел в топологию
        topology.add_id(iid);
        std::string parent_port;

        // Выбираем порт для подключения нового узла к родителю
        if (topology.get_parent_id(iid) == -1) {
            parent_port = port;
        } else {
            parent_port = get_free_port();
        }

        // Пингуем родителя, чтобы убедиться, что он существует
        if (topology.get_parent_id(iid) != -1) {
            std::string ping_response = ping(std::to_string(topology.get_parent_id(iid)));
            if (ping_response == "Ok: 0") {
                return "Error: Parent is unavailable";
            }
            if (ping_response == "Error: Not found") {
                return "Error: Parent not found";  // По сути, эта ошибка никогда не будет выведена
            }
        }

        // Отправляем родителю сообщение, чтобы он забиндил свой сокет на нужный порт
        if (parent_port != port) {
            std::string side = (topology.is_left_child(iid) ? "left" : "right");
            std::string bind_response = bind(topology.get_parent_id(iid), side, parent_port);

            if (bind_response == "Error") {
                return "Error: Parent cant bind " + side + " socket to port " + parent_port;
            }
        }

        // Создаём процесс вычислительного узла
        pid_t pid = fork();
        if (pid == -1) {
            return "Error: Fork failed";
        }
        if (pid == 0) {
            // Дочерний процесс
            execl("./src/server", id.c_str(), parent_port.c_str(), NULL);

            return "Error: The computing node process has not started";
        } else {
            // Родительский процесс
            return "Ok: " + std::to_string(pid);
        }
    }

    std::string exec(const std::string& id, const std::string& args) {
        std::string ping_response = ping(id);
        if (ping_response != "Ok: 1") {
            return "Error:" + id + ": Node is unavailable";
        }
    
        return request_helper(id, "exec", {args});
    }

    // Обработка пользовательских команд
    void process_user_command(const std::string& command_message) {
        std::string cmd;
        std::string id;
        std::istringstream iss(command_message);
        iss >> cmd;
        if (cmd == "exit") {
            running = false;
            return;
        }
        iss >> id;
        if (cmd == "create") {
            futures.push_back(std::async(
                std::launch::async,
                [this, id](){
                    std::cout << create(id) << std::endl;
                }
            ));
        } else if (cmd == "ping") {
            futures.push_back(std::async(
                std::launch::async,
                [this, id](){
                    std::cout << ping(id) << std::endl;
                }
            ));
        } else if (cmd == "exec") {
            std::string args;
            std::getline(iss, args);
            futures.push_back(std::async(
                std::launch::async,
                [this, id, args](){
                    std::cout << exec(id, args) << std::endl;
                }
            ));
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

    // Обработка сообщенинй от системы
    void process_incoming_message(const std::string& message) {
        std::vector<std::string> tokens = split(message, ';');
        int request_id = std::stoi(tokens[0]);
        std::string answer = tokens[1];

        if (request_promise_map.count(request_id)) {
            request_promise_map[request_id].set_value(answer);
            request_promise_map.erase(request_id);
        }
    }

public:
    ControlNode(const std::string& port)
    : topology()
    , context(1)
    , port(port)
    , socket(context, ZMQ_PAIR)
    , request_promise_map()
    , futures()
    , running(true)
    {
        socket.bind("tcp://*:" + port);

        // Устанавливаем параметры сокета
        socket.set(zmq::sockopt::linger, 2000);   // Удаление сообщений, если отправить нельзя
        socket.set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
        socket.set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку
    }

    void run() {
        zmq::pollitem_t items[] = {
            {socket, 0, ZMQ_POLLIN, 0}
        };

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Опрашиваем сокет на наличие сообщений
            int events = zmq::poll(items, 1, std::chrono::milliseconds(0)); // Немедленное возвращение
            if (events < 0) {
                std::cerr << "zmq::poll failed" << std::endl;
                continue;
            }

            if (items[0].revents & ZMQ_POLLIN) {
                std::optional<std::string> message = Receive(socket);
                if (message.has_value()) {
                    // std::cout << "Processing system message: " << message.value() << std::endl;
                    process_incoming_message(message.value());
                } else {
                    std::cerr << "Failed to receive message" << std::endl;
                }
            }

            // Обработка пользовательских команд
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            struct timeval timeout = {0, 0}; // Немедленное возвращение
            if (select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout) > 0) {
                std::string request;
                std::getline(std::cin, request);

                // std::cout << "Processing user request: " << request << std::endl;
                process_user_command(request);
            }
        }

        for (auto& future : futures) {
            future.get();
        }
    }


    ~ControlNode() {
        std::cout << "Destroying System..." << std::endl;

        try {
            if (socket.handle() != nullptr) {
                Send(socket, "recursive_destroy");
                // socket.disconnect("tcp://localhost:" + port);
                socket.close(); // Закрытие сокета
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "Error while destroying ControlNode: " << e.what() << std::endl;
        }

        context.close(); // Закрытие контекста
        std::cout << "System destroyed." << std::endl;
    }
};