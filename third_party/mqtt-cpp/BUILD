load("@bazel_skylib//rules:build_test.bzl", "build_test")

exports_files(
    [
        "BUILD.mqtt-cpp",
    ],
    visibility = ["//visibility:public"],
)

build_test(
    name = "mqtt-cpp_build_test",
    targets = [
        "@mqtt-cpp",
    ],
    visibility = ["//:__pkg__"],
)
