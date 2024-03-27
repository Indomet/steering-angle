#include <iostream>
#include "PrimeChecker.hpp"

int main(int argc, char** argv) {
    //checking if there are exactly 2 arguments
    if (argc == 2) {
        //get first arg
        int number = std::stoi(argv[1]);
        //create an object
        PrimeChecker pc;
        std::cout << "Altinisik, Mehmet Asim" << number << " is a prime number? " << pc.isPrime(number) << std::endl;
    }
    return 0;
}