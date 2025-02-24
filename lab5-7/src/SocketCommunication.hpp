#pragma once
#include <zmq.hpp>
#include <optional>
#include <chrono>

std::string get_timestamp() {
    // Получение текущего времени в миллисекундах
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // Преобразование в строку
    std::string millisStr = std::to_string(millis);

    // Получение последних 6 цифр
    if (millisStr.size() > 6) {
        return millisStr.substr(millisStr.size() - 6);
    } else {
        return millisStr; // Если меньше 6 цифр, возвращаем всю строку
    }
}

bool Send(zmq::socket_t& receiver, std::string message) {
    zmq::message_t zmes(message.size());
    memcpy(zmes.data(), message.c_str(), message.size());
    
    bool result = false;
    try {
        result = receiver.send(zmes, zmq::send_flags::dontwait).has_value();
    } catch (const zmq::error_t& e) {
        // Если произошла ошибка (например, сокет недоступен), выводим сообщение
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }

    std::cout << "    " << get_timestamp() << "-send{" << message << "}:";
    std::cout << (result ? "SUCCESSFUL" : "FAILED") << std::endl;

    return result;
}

std::optional<std::string> Receive(zmq::socket_t& sender) {
    zmq::message_t request;
    if (sender.recv(request, zmq::recv_flags::none).has_value()) {
        std::cout << "    " << get_timestamp() << "-recv{" << request.to_string() << "}" << std::endl;
        return {request.to_string()};
    }
    return std::nullopt;
}