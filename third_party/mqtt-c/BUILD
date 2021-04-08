load("@bazel_skylib//rules:build_test.bzl", "build_test")

exports_files(
    [
        "BUILD.mqtt-c",
    ],
    visibility = ["//visibility:public"],
)

build_test(
    name = "mqtt-c_build_test",
    targets = [
        "@mqtt-c//:mqtt-c",
    ],
    visibility = ["//:__pkg__"],
)
