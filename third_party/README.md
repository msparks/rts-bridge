# Third-party packages

This directory follows the basic structure from
[`bazelbuild/rules_foreign_cc/examples/third_party`]. Each subdirectory contains
two `BUILD` files. For example, under [`openssl`](openssl):

- `BUILD`: Standard `BUILD` file that declares `third_party/openssl` as a
  package. It exports the `BUILD.openssl` file as a target in the package to be
  referenced from the `WORKSPACE` file at the root.
- `BUILD.openssl`: Referenced by the `http_archive` rule in `WORKSPACE` to use
  as the *overlay* `BUILD` file. It's applied on top of the openssl codebase
  when it is fetched by Bazel. This `BUILD` file defines the `:openssl` target
  using a `configure_make` rule, which describes to Bazel how to build openssl,
  and what the outputs are.

The rule in `WORKSPACE` to declare openssl as
an [external repository](https://docs.bazel.build/versions/master/external.html)
looks like:

```
http_archive(
    name = "openssl",
    build_file = "//third_party/openssl:BUILD.openssl",
    sha256 = "23011a5cc78e53d0dc98dfa608c51e72bcd350aa57df74c5d5574ba4ffb62e74",
    strip_prefix = "openssl-OpenSSL_1_1_1d",
    url = "https://github.com/openssl/openssl/archive/OpenSSL_1_1_1d.tar.gz",
)
```

Only `WORKSPACE` files can define repositories. It's possible to put the details
of the repository in a `.bzl` file alongside the `BUILD` files and call into it
from `WORKSPACE`, but that isn't done here just for simplicity.

Targets within an external repository are referenced using an `@`, e.g.,
`@openssl//...`. Because the `configure_make` target in the overlay file,
`BUILD.openssl`, is named `openssl`, the full target name is
`@openssl//:openssl`. Or just `@openssl` for short, since the target name is the
same as the repository.

The packages in `third_party` are necessary only for external dependencies that
don't use Bazel natively. Google's projects like [Abseil](https://abseil.io),
for example, have `BUILD` files that can be referenced after including
`http_archive` in the `WORKSPACE`.

Once a third-party package is defined, use it by adding a dependency on the
`BUILD` target:

```
cc_binary(
    name = "foo",
    srcs = ["foo.cc"],
    deps = [
      "@openssl",
    ],
)
```

In the code, the file structure for the `#include`s is project-dependent. Refer
to the library documentation.

[`bazelbuild/rules_foreign_cc/examples/third_party`]: https://github.com/bazelbuild/rules_foreign_cc/tree/main/examples/third_party).
