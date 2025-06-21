{
  "targets": [
    {
      "target_name": "midicci_bridge",
      "sources": [
        "src/bridge.cpp",
        "src/ci_tool_repository_wrapper.cpp",
        "src/ci_device_manager_wrapper.cpp",
        "src/midi_device_manager_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../ci-tool/include",
        "../../src"
      ],
      "libraries": [
        "../build/tools/ci-tool/libmidicci-tooling.a",
        "../build/src/libmidicci.a"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      },
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }
  ]
}
