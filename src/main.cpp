#include <iostream>
#include <string>

int main() {
    std::cout << "Kung Fu Chess\n";
    std::cout << "Enter something: ";

    std::string input;
    std::getline(std::cin, input);

    std::cout << "You entered: " << input << "\n";
    return 0;
}
