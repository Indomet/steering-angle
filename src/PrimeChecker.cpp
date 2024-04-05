#include "PrimeChecker.hpp"

bool PrimeChecker::isPrime(uint16_t n) {
    bool retVal { true };
    if (n < 2) {
        return false;
    } else if (n == 2) {
        return true;
    } else if (n % 2 == 0) {
        return false;
    } else {
        for (uint16_t i { 3 }; (i * i) <= n; i += 2) {
            if (n % i == 0) {
                return false;
            }
        }
    }
    return true;
}
