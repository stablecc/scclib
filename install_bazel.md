# installing bazel

An easy way to install in Linux is to use the standalone installer (available with version 5.0.0).
For example, on x86_64 Linux:
```
wget https://github.com/bazelbuild/bazel/releases/download/5.0.0/bazel-5.0.0-installer-linux-x86_64.sh
chmod +x bazel-5.0.0-installer-linux-x86_64.sh 
```

Full installation details are on [bazel.build](https://bazel.build/install).

## local user install

Run the installer in user mode
```
./bazel-5.0.0-installer-linux-x86_64.sh --user
```

Make sure you have $HOME/bin in your path, for example by adding the following to $HOME/.bashrc
```
echo 'export PATH=$HOME/bin:$PATH' >> $HOME/.bashrc
source $HOME/.bashrc
```

To enable bash completion
```
echo 'source $HOME/.bazel/bin/bazel-complete.bash' >> $HOME/.bashrc
source $HOME/.bashrc
```

## global install

Run the installer in root mode
```
sudo ./bazel-5.0.0-installer-linux-x86_64.sh
```

The bazel executable will be installed in /usr/local/bin.

## bazelrc

Optionally, can set up some debugging defaults in ```.bazelrc```, for example:
```
# --compilation_mode=<fastbuild, dbg or opt> [-c] default: "fastbuild"
build --compilation_mode=dbg

# --[no]subcommands [-s] (true, pretty_print or false; default: "false")
build --subcommands=true

# --[no]verbose_failures (a boolean; default: "false")
build --verbose_failures
```
