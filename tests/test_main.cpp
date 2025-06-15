#include <iostream>

int run_all_tests();

int main() {
    std::cout << "Running MIDI-CI tests...\n";
    
    int result = run_all_tests();
    
    if (result == 0) {
        std::cout << "All tests passed!\n";
    } else {
        std::cout << "Some tests failed!\n";
    }
    
    return result;
}
