var index =
[
    [ "Table of Contents", "index.html#autotoc_md1", null ],
    [ "Introduction", "index.html#autotoc_md2", [
      [ "Getting Started", "index.html#autotoc_md4", null ],
      [ "Flexible Packaging: Packaging required modules", "index.html#autotoc_md6", null ],
      [ "Project Structure", "index.html#autotoc_md8", null ]
    ] ],
    [ "Userspace Resource Manager Key Points", "index.html#autotoc_md10", [
      [ "Resource Tuner", "index.html#autotoc_md11", null ],
      [ "Contextual Classifier", "index.html#autotoc_md12", null ]
    ] ],
    [ "System Independent Layer", "index.html#autotoc_md14", [
      [ "Logical IDs", "index.html#autotoc_md15", [
        [ "1. Logical Cluster Map", "index.html#autotoc_md16", null ],
        [ "2. Cgroups map", "index.html#autotoc_md17", null ],
        [ "3. Mpam Groups Map", "index.html#autotoc_md18", null ]
      ] ],
      [ "Resources", "index.html#autotoc_md19", null ],
      [ "Signals", "index.html#autotoc_md20", null ]
    ] ],
    [ "APIs", "index.html#autotoc_md21", [
      [ "tuneResources", "index.html#autotoc_md22", null ],
      [ "retuneResources", "index.html#autotoc_md24", null ],
      [ "untuneResources", "index.html#autotoc_md26", null ],
      [ "tuneSignal", "index.html#autotoc_md28", null ],
      [ "untuneSignal", "index.html#autotoc_md30", null ],
      [ "relaySignal", "index.html#autotoc_md32", null ],
      [ "getProp", "index.html#autotoc_md34", null ]
    ] ],
    [ "Config Files Format", "index.html#autotoc_md36", [
      [ "1. Initialization Configs", "index.html#autotoc_md37", [
        [ "Common Initialization Configs", "index.html#autotoc_md38", null ],
        [ "Overriding Initialization Configs", "index.html#autotoc_md39", null ],
        [ "Overiding with Custom Extension File", "index.html#autotoc_md40", null ],
        [ "1. Logical Cluster Map", "index.html#autotoc_md41", null ],
        [ "2. Cgroups map", "index.html#autotoc_md42", null ],
        [ "3. Mpam Groups Map", "index.html#autotoc_md43", [
          [ "Fields Description for CGroup Config", "index.html#autotoc_md44", null ],
          [ "Fields Description for Mpam Group Config", "index.html#autotoc_md45", null ],
          [ "Fields Description for Cache Info Config", "index.html#autotoc_md46", null ],
          [ "Example", "index.html#autotoc_md47", null ]
        ] ]
      ] ],
      [ "2. Resource Configs", "index.html#autotoc_md49", [
        [ "Common Resource Configs", "index.html#autotoc_md50", null ],
        [ "Overriding Resource Configs", "index.html#autotoc_md51", null ],
        [ "Overiding with Custom Extension File", "index.html#autotoc_md52", [
          [ "Fields Description", "index.html#autotoc_md53", null ],
          [ "Example", "index.html#autotoc_md54", null ]
        ] ]
      ] ],
      [ "3. Properties Config", "index.html#autotoc_md56", [
        [ "Common Properties Configs", "index.html#autotoc_md57", null ],
        [ "Overriding Properties Configs", "index.html#autotoc_md58", null ],
        [ "Overiding with Custom Properties File", "index.html#autotoc_md59", [
          [ "Field Descriptions", "index.html#autotoc_md60", null ],
          [ "Example", "index.html#autotoc_md61", null ]
        ] ]
      ] ],
      [ "4. Signal Configs", "index.html#autotoc_md62", null ],
      [ "5. (Optional) Target Configs", "index.html#autotoc_md65", null ],
      [ "6. (Optional) ExtFeatures Configs", "index.html#autotoc_md68", null ]
    ] ],
    [ "Resource Format", "index.html#autotoc_md72", [
      [ "Notes on Resource ResCode", "index.html#autotoc_md73", null ]
    ] ],
    [ "Example Usage of Resource Tuner APIs", "index.html#autotoc_md76", [
      [ "tuneResources", "index.html#autotoc_md77", null ],
      [ "retuneResources", "index.html#autotoc_md78", null ],
      [ "untuneResources", "index.html#autotoc_md79", null ]
    ] ],
    [ "Extension Interface", "index.html#autotoc_md81", [
      [ "Extension APIs", "index.html#autotoc_md82", [
        [ "RESTUNE_REGISTER_APPLIER_CB", "index.html#autotoc_md83", null ],
        [ "Usage Example", "index.html#autotoc_md84", null ],
        [ "RESTUNE_REGISTER_TEAR_CB", "index.html#autotoc_md86", null ],
        [ "Usage Example", "index.html#autotoc_md87", null ],
        [ "RESTUNE_REGISTER_CONFIG", "index.html#autotoc_md89", null ],
        [ "Usage Example", "index.html#autotoc_md90", null ],
        [ "Usage Example", "index.html#autotoc_md91", null ]
      ] ]
    ] ],
    [ "Resource Tuner Features", "index.html#autotoc_md93", [
      [ "Initialization", "index.html#autotoc_md94", null ],
      [ "Request Processing", "index.html#autotoc_md95", null ],
      [ "1. Client-Level Permissions", "index.html#autotoc_md97", null ],
      [ "2. Resource-Level Policies", "index.html#autotoc_md98", null ],
      [ "3. Request-Level Priorities", "index.html#autotoc_md99", null ],
      [ "4. Pulse Monitor: Detection of Dead Clients and Subsequent Cleanup", "index.html#autotoc_md100", null ],
      [ "5. Rate Limiter: Preventing System Abuse", "index.html#autotoc_md101", null ],
      [ "6. Duplicate Checking", "index.html#autotoc_md102", null ],
      [ "7. Dynamic Mapper: Logical to Physical Mapping", "index.html#autotoc_md103", null ],
      [ "8. Display-Aware Operational Modes", "index.html#autotoc_md104", null ],
      [ "9. Crash Recovery", "index.html#autotoc_md105", null ],
      [ "10. Flexible Packaging", "index.html#autotoc_md106", null ],
      [ "11. Pre-Allocate Capacity for efficiency", "index.html#autotoc_md107", null ]
    ] ],
    [ "Client CLI", "index.html#autotoc_md109", [
      [ "Usage Examples", "index.html#autotoc_md110", [
        [ "1. Send a Tune Request", "index.html#autotoc_md111", null ],
        [ "2. Send an Untune Request", "index.html#autotoc_md112", null ],
        [ "3. Send a Retune Request", "index.html#autotoc_md113", null ],
        [ "4. Send a getProp Request", "index.html#autotoc_md114", null ],
        [ "5. Send a tuneSignal Request", "index.html#autotoc_md115", null ]
      ] ]
    ] ],
    [ "Contact", "index.html#autotoc_md116", null ],
    [ "License", "index.html#autotoc_md117", null ]
];