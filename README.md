# mizzou_dining_info
Command-line version for testing purposes.

## NOTE
`httplib` is already compiled to `httplib.o` and there should be no reason to re-compile it. If this is desired for some reason, it can be re-compiled with `build_httplib.sh`.

## Compiling the command line tool
`httplib` requires an older version of openssl, 1.1.1. These instructions assume that openssl version 3 has already been installed with `brew`, which is the case on most systems with `brew`.

First, install the older 1.1.1 version of openssl (alongside version 3):
```
brew install openssl@1.1
```

Then, temporarily unlink version 3 so that `clang` can find 1.1.1:
```
brew unlink openssl@3
```

Finally, you can compile the program:
```
./build.sh
```

When you're done messing around with the command line version, make sure re-link the newer version of openssl so other stuff doesn't break:
```
brew link openssl@3
```
