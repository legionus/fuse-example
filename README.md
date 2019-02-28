# Build:
```
mkdir build
cd build
cmake ..
make
```

# How to crash the kernel:


```
# If you have unprivileged user namespaces, you can do this under a userns, otherwise you need root
unshare -U -r # Create a user namespace
unshare --mount-proc -p --propagation=private --fork /bin/bash # This gives us a private pid namespace
mkdir -p /tmp/example
./bin/fuse-example -s -f /tmp/example &
cat /tmp/example/fire
```

## Explanation
Basically, what’s happening is that the FUSE daemon which is the one
handling the FUSE requests has /dev/fuse open. There is a thread in
that FUSE daemon. let’s call the FUSE daemon P10, and the thread P11.

The example starts, and mounts a FUSE filesystem on /tmp/example.
It is launched with one process, which only has one thread, PID X, TID 0.

When we cat /tmp/example/fire, PID X spawns TID 2, and replies to the
call. PID X, TID 2 opens /tmp/example/file, and calls fsync on it.
This initiated a call into kernel space, and subsequently, a the op
gets pushed back into userspace over to the FD that PID X, TID 0
has open. That process reads the op, and then calls reboot, terminating
pid 1.

When pid 1 terminates, PID X, TID 0 is running userspace code,
and can be send a signal, and subsequently terminated. Therefore,
it will never get a chance to reply to the op in flight, and that
op will stay in userspace.

/dev/fuse will not be closed, because PID X, TID 2 is still running,
and it is in an uninterruptible sleep. That uninterruptible sleep
will never be finished, as it's in `request_wait_answer`. This makes
it so that the mount namespace, nor the pid namespace will be
torn down. This, in turn results in the FUSE connection never
being aborted.
