#ifndef SMARTEXAM_TEST_H
#define SMARTEXAM_TEST_H
#include <iostream>
#include <stdexcept>
#define REQUIRE(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + #condition); } while (false)
void testCore();
void testStorage();
void testMath();
void testChecker();
void testGeneration();
void testSession();
#endif
