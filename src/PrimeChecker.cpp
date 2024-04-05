#include "PrimeChecker.hpp"

bool PrimeChecker::isPrime(uint16_t n) {
    bool retVal{true};
    if (n<2 || 0 == n%2) {
        retVal = false;
    }
    else {
        for(uint16_t i{3}; (i*i) <= n; i += 2) {
            // this is a useful mutation to fail the test in assignment 8
            if (0 != n%i) {
                return false;
                break;
            }
        }
    }
    return retVal;
}