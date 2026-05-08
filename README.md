# MetaCore

Provides way too many utilities for mod developers.

The main idea for this came from creating Qounters++ and its API, and felt like many of the statistics and events it provided could be useful without needing any ties to Qounters++ itself. From there, I thought of every bit of reused code across mods I had, and inter-mod communications I could facilitate, and now we're here.

For easier updates, and the likely never to be realized idea of using this for other games, all Beat Saber specific utilities are now in [MetaCoreBS](https://github.com/Metalit/MetaCoreBS).

## Documentation

This is just the general idea for each header file with the general contents to expect for each. Documentation for specific functions and variables can be found in the files themselves. There is also more in [MetaCoreBS](https://github.com/Metalit/MetaCoreBS).

### `assets.cmake`

Automatically includes any assets in the top-level `assets` directory of your project into the compiled object file. Use by adding this to your `CMakeLists.txt`:

```cmake
include(extern/includes/metacore/shared/assets.cmake)
```

### `assets.hpp`

Provides definitions and utilities for assets included using the cmake script.

### `delegates.hpp`

Improves on BSML's delegate helpers to make even easier (less verbose) delegate creation, specifically around lambdas.

### `il2cpp.hpp`

Provides experimental utilities and wrappers for Il2Cpp types.

### `java.hpp`

Allows easier and less verbose use of java and JNI functions.

### `jutils.hpp`

Provides some base types used in `java.hpp`, with more detailed documentation in their use.

### `maps.hpp`

Defines two potentially useful std::map wrappers for different types of data access and storage.

### `operators.hpp`

Provides magic macros that make operators usable on cordl types. (Credit to [Qwasyx](https://github.com/Qwasyx).)

### `unity.hpp`

Provides various functions to enhance working with Unity Components, Transforms, Quaternions, and other objects. Namespace is named `Engine` instead of `Unity` to avoid collisions.
