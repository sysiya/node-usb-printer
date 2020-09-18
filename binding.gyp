{
  "targets": [
    {
      "target_name": "addon", # addon.node
      "sources": [ "src/*.cc" ], # entry
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"], # where to find dependencies
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"], # need add into .node
      "cflags_cc": [
        "-std=c++17" # use c++17 standard
      ],
      "cflags!": [ "-fno-exceptions" ], # disable exceptions
      "cflags_cc!": [ "-fno-exceptions" ],  # disable exceptions
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "NODE_ADDON_API_DISABLE_DEPRECATED" ],
      "conditions": [
        [
          "OS=='win'",
          {
            "libraries": [
                "setupapi.lib", 
            ],
          }
        ]
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "AdditionalOptions": [ "/EHsc"] 
        }
      }
    }
  ]
}