# Setup

```shell
cmake -S . -B path/to/build -G Ninja
cmake --build path/to/build
ln -s path/to/build/compile_commands.json compile_commands.json
```

## CLion

From the direnv environment, run the following command. It will have to be rerun every time the environment is changed.

```shell
mkdir -p tools
for tool in cmake ninja; do
    echo '#!/usr/bin/env bash' > "tools/${tool}"
    env | sed -E 's/^([^=]+)=(.*)$/export \1="\2"/' >> "tools/${tool}"
    echo "${tool} \"\$@\"" >> "tools/${tool}"
    chmod +x "tools/${tool}"
done
```

Then in CLion's settings, use `tools/cmake` and `tools/ninja` to configure a toolchain.
