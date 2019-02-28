#define FUSE_USE_VERSION 26
#define _GNU_SOURCE 1

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/reboot.h>

static const char *filepath = "/file";
static const char *filename = "file";
static const char *fire = "fire";
static const char *firepath = "/fire";
static const char *filecontent = "I'm the content of the only file available there\n";

int fired = 0;
void *bad_bad(void *vargp) {
  int fd = open("/tmp/example/file", O_RDONLY);
  pid_t tid = syscall(SYS_gettid);
  char buf[1024];

  printf("Opened fd: %d in tid %d\n", fd, tid);
  fsync(fd);
  printf("Completed\n");
}

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (strcmp(path, filepath) == 0 || strcmp(path, firepath) == 0) {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(filecontent);
    return 0;
  }

  return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  filler(buf, filename, NULL, 0);
  filler(buf, fire, NULL, 0);

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int fsync_callback(const char *path, int x, struct fuse_file_info *fi) {
  printf("performing fsync\n");
  syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF);
  printf("Did reboot\n");
  pause();
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
  pthread_t thread_id;

  if (strcmp(path, filepath) == 0) {
    size_t len = strlen(filecontent);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  if (strcmp(firepath, path) == 0) {
    if (!fired) {
      fired = 1;
      printf("Triggering fire\n");
      pthread_create(&thread_id, NULL, bad_bad, NULL);
    }
  }

  return -ENOENT;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
  .fsync = fsync_callback,
};

int main(int argc, char *argv[])
{

  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
