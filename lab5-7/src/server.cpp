#include "TreeNode.hpp"

int main(int argc, char* argv[]) {
    // Разбираем параметры
    int id = atoi(argv[0]);
    std::string parent_port(argv[1]);

    // Создаём объект вычислительного узла
    TreeNode node(id);
    node.setup_parent_socket(parent_port);

    // Запускаем его
    node.run();
}