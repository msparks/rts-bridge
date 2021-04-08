load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# https://github.com/bazelbuild/rules_foreign_cc
http_archive(
    name = "rules_foreign_cc",
    sha256 = "d54742ffbdc6924f222d2179f0e10e911c5c659c4ae74158e9fe827aad862ac6",
    strip_prefix = "rules_foreign_cc-0.2.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.2.0.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

# This sets up some common toolchains for building targets.
rules_foreign_cc_dependencies()

# https://github.com/openssl/openssl
http_archive(
    name = "openssl",
    build_file = "//third_party/openssl:BUILD.openssl",
    sha256 = "23011a5cc78e53d0dc98dfa608c51e72bcd350aa57df74c5d5574ba4ffb62e74",
    strip_prefix = "openssl-OpenSSL_1_1_1d",
    url = "https://github.com/openssl/openssl/archive/OpenSSL_1_1_1d.tar.gz",
)

# https://github.com/eclipse/paho.mqtt.c
http_archive(
    name = "mqtt-c",
    build_file = "//third_party/mqtt-c:BUILD.mqtt-c",
    sha256 = "9a19f904e876d5257eac8cf61cc15ced09fb7d1296143b3b9b4bcb4ea94616ae",
    strip_prefix = "paho.mqtt.c-1.3.8",
    url = "https://github.com/eclipse/paho.mqtt.c/archive/v1.3.8.zip",
)

# https://github.com/eclipse/paho.mqtt.cpp
http_archive(
    name = "mqtt-cpp",
    build_file = "//third_party/mqtt-cpp:BUILD.mqtt-cpp",
    sha256 = "90c4d8ae4f56bb706120fddcc5937cd0a0360b6f39d5cd5574a5846c0f923473",
    strip_prefix = "paho.mqtt.cpp-1.2.0",
    url = "https://github.com/eclipse/paho.mqtt.cpp/archive/v1.2.0.zip",
)

# https://github.com/abseil/abseil-cpp
http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-98eb410c93ad059f9bba1bf43f5bb916fc92a5ea",
    url = "https://github.com/abseil/abseil-cpp/archive/98eb410c93ad059f9bba1bf43f5bb916fc92a5ea.zip",
)

# https://github.com/google/googletest
http_archive(
    name = "com_google_googletest",
    sha256 = "6a5d7d63cd6e0ad2a7130471105a3b83799a7a2b14ef7ec8d742b54f01a4833c",
    strip_prefix = "googletest-011959aafddcd30611003de96cfd8d7a7685c700",
    url = "https://github.com/google/googletest/archive/011959aafddcd30611003de96cfd8d7a7685c700.zip",
)

# https://github.com/google/benchmark
http_archive(
    name = "com_github_google_benchmark",
    sha256 = "21e6e096c9a9a88076b46bd38c33660f565fa050ca427125f64c4a8bf60f336b",
    strip_prefix = "benchmark-1.5.2",
    url = "https://github.com/google/benchmark/archive/refs/tags/v1.5.2.zip",
)
