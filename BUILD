package(licenses = ["notice"])

exports_files([
    "LICENSE",
    "NOTICE",
])

cc_binary(
    name = "ssstencil",
    srcs = ["src/main.cc"],
    copts = ["-UNDEBUG"],
    deps = [
        "@libprim//:prim",
        "@tclap//:tclap",
    ],
    visibility = ["//visibility:public"],
)

genrule(
    name = "lint",
    srcs = glob([
        "src/**/*.cc",
        "src/**/*.h",
        "src/**/*.tcc",
    ]),
    outs = ["linted"],
    cmd = """
    python $(location @cpplint//:cpplint) \
      --root=$$(pwd) \
      --headers=h,tcc \
      --extensions=cc,h,tcc \
      --quiet $(SRCS) > $@
    echo // $$(date) > $@
    """,
    tools = [
        "@cpplint",
    ],
    visibility = ["//visibility:public"],
)

genrule(
    name = "format_check",
    srcs = glob([
        "src/**/*.cc",
        "src/**/*.h",
        "src/**/*.tcc",
    ]),
    outs = ["format_checked"],
    cmd = """
    cp $(location @clang_format//file) .clang-format
    clang-format --style=file --dry-run --Werror $(SRCS)
    echo // $$(date) > $@
    """,
    tools = [
        "@clang_format//file",
    ],
    visibility = ["//visibility:public"],
)
