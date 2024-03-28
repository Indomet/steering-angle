#include <iostream>
#include "PrimeChecker.hpp"

int main(int argc, char** argv) {
    //checking if there are exactly 2 arguments
    if (argc == 2) {
        //get first arg
        int number = std::stoi(argv[1]);
        //create an object
        PrimeChecker pc;
        // Prints the result of the prime number check for the given number.
        std::cout << "Latos, Georgios" << number << " is a prime number? " << pc.isPrime(number) << std::endl;
    }
    return 0;
}