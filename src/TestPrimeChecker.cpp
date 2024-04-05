#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this once per test-runner!

#include "PrimeChecker.hpp"
#include "catch.hpp"

TEST_CASE("Test PrimeChecker 1.") {
    PrimeChecker pc;
    REQUIRE(pc.isPrime(2)); // 2 is a prime number
}

TEST_CASE("Test PrimeChecker 2.") {
    PrimeChecker pc;
    REQUIRE(pc.isPrime(3)); // 3 is a prime number
}

// This test case should fail
TEST_CASE("Test PrimeChecker 3.") {
    PrimeChecker pc;
    REQUIRE(pc.isPrime(4)); // 4 is not a prime number
}
