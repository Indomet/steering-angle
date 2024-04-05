#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this once per test-runner!

#include "PrimeChecker.hpp"
#include "catch.hpp"

TEST_CASE("Test PrimeChecker 1.") {
    PrimeChecker pc;
    REQUIRE(pc.isPrime(5));
}

// This test case should fail
TEST_CASE("Test PrimeChecker 2.") {
    PrimeChecker pc;
    REQUIRE(pc.isPrime(4));
}