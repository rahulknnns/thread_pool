#include <iostream>
#include "thread_pool.hpp"


float add(float a, float b) {
    return a + b;
};

float multiply(float a, float b) {
    return a * b;
};

int main() {
    ThreadPool pool(4); // Create a thread pool with 4 threads

    // Submit tasks to the thread pool
    auto future1 = pool.submit(add, 1.0f, 2.0f);
    auto future2 = pool.submit(multiply, 3.0f, 4.0f);

    // Get results from future
   std::cout << "Waiting for results..." << std::endl;
    std::cout << "Result of add: " << future1.get() << std::endl;        // Should print 3.0
    std::cout << "Result of multiply: " << future2.get() << std::endl; // Should print 12.0
    // The thread pool will automatically clean up when it goes out of scope

    return 0;
}