// Unit testing oqs::KeyEncapsulation

#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "oqs_cpp.h"

// no_thread_KEMs lists KEMs that have issues running in a separate thread
static std::vector<std::string> no_thread_KEMs{
    "Classic-McEliece-348864",  "Classic-McEliece-348864f",
    "Classic-McEliece-460896",  "Classic-McEliece-460896f",
    "Classic-McEliece-6688128", "Classic-McEliece-6688128f",
    "Classic-McEliece-6960119", "Classic-McEliece-6960119f",
    "Classic-McEliece-8192128", "Classic-McEliece-8192128f",
    "LEDAcryptKEM-LT52"};

// used for thread-safe console output
static std::mutex mu;

void test_kem(const std::string& kem_name) {
    {
        std::lock_guard<std::mutex> lg{mu};
        std::cout << kem_name << std::endl;
    }
    oqs::KeyEncapsulation client{kem_name};
    oqs::bytes client_public_key = client.generate_keypair();
    oqs::KeyEncapsulation server{kem_name};
    oqs::bytes ciphertext, shared_secret_server;
    std::tie(ciphertext, shared_secret_server) =
        server.encap_secret(client_public_key);
    oqs::bytes shared_secret_client = client.decap_secret(ciphertext);
    bool is_valid = (shared_secret_client == shared_secret_server);
    if (!is_valid)
        std::cerr << kem_name << ": shared secrets do not coincide"
                  << std::endl;
    EXPECT_TRUE(is_valid);
}

TEST(oqs_KeyEncapsulation, Enabled) {
    std::vector<std::thread> thread_pool;
    std::vector<std::string> enabled_KEMs = oqs::KEMs::get_enabled_KEMs();
    for (auto&& kem_name : enabled_KEMs) {
        // use threads only for KEMs that are not in no_thread_KEMs, due to
        // issues with stack size being too small in macOS (512Kb for threads)
        if (std::find(std::begin(no_thread_KEMs), std::end(no_thread_KEMs),
                      kem_name) == std::end(no_thread_KEMs))
            thread_pool.emplace_back(test_kem, kem_name);
    }
    // test the other KEMs in the main thread (stack size is 8Mb on macOS)
    for (auto&& kem_name : no_thread_KEMs)
        if (std::find(std::begin(enabled_KEMs), std::end(enabled_KEMs),
                      kem_name) != std::end(enabled_KEMs))
            test_kem(kem_name);
    // join the rest of the threads
    for (auto&& elem : thread_pool)
        elem.join();
}

TEST(oqs_KeyEncapsulation, NotSupported) {
    EXPECT_THROW(oqs::KeyEncapsulation{"unsupported_kem"},
                 oqs::MechanismNotSupportedError);
}
