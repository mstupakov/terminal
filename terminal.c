#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>

#define SOH 1   /* sender start of block header        */
#define EOT 4   /* sender end of block transfer        */
#define ACK 6   /* target block ack                    */
#define NAK 21  /* target block negative ack           */
#define CAN 24  /* target/sender transfer cancellation */

static speed_t tty_speed(int baudrate) {

  switch (baudrate) {
    case 115200: return B115200;
    case 57600 : return B57600;
    case 38400 : return B38400;
    case 19200 : return B19200;
    case 9600  : return B9600;
  }

  return -1;
}

static int open_tty(const char *path, speed_t speed) {
  int rc = -1, fd;
  struct termios tio;

  fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    goto out;
  }

  memset(&tio, 0, sizeof(tio));

  tio.c_iflag = 0;
  tio.c_cflag = CREAD|CLOCAL|CS8;

  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 10;

  cfsetospeed(&tio, speed);
  cfsetispeed(&tio, speed);

  rc = tcsetattr(fd, TCSANOW, &tio);
  if (rc) {
    goto out;
  }

  rc = fd;

out:
  if (rc < 0) {
    if (fd >= 0) {
      close(fd);
    }
  }

  return rc;
}

static int term_pipe(int in, int out, char *quit, int *s) {
  ssize_t nin, nout;
  char _buf[128], *buf = _buf;

  nin = read(in, buf, sizeof(buf));
  if (nin < 0) {
    return -1;
  }

  if (quit) {
    int i;

    for (i = 0; i < nin; i++) {
      if (*buf == quit[*s]) {
        (*s)++;
        if (!quit[*s]) {
          return 0;
        }

        buf++;
        nin--;
      } else {
        while (*s > 0) {
          nout = write(out, quit, *s);
          if (nout <= 0)
            return -1;
          (*s) -= nout;
        }
      }
    }
  }

  while (nin > 0) {
    nout = write(out, buf, nin);
    if (nout <= 0) {
      return -1;
    }

    nin -= nout;
  }

  return 0;
}

static int terminal(int tty) {
  int rc = -1, in, s;
  char *quit = "\34c";
  struct termios otio, tio;

  in = STDIN_FILENO;
  if (isatty(in)) {
    rc = tcgetattr(in, &otio);
    if (!rc) {
      tio = otio;
      cfmakeraw(&tio);
      rc = tcsetattr(in, TCSANOW, &tio);
    }
    if (rc) {
      perror("tcsetattr");
      goto out;
    }

  } else {
    in = -1;
  }

  rc = 0;
  s = 0;

  do {
    fd_set rfds;
    int nfds = 0;

    FD_SET(tty, &rfds);
    nfds = nfds < tty ? tty : nfds;

    if (in >= 0) {
      FD_SET(in, &rfds);
      nfds = nfds < in ? in : nfds;
    }

    nfds = select(nfds + 1, &rfds, NULL, NULL, NULL);
    if (nfds < 0) {
      break;
    }

    if (FD_ISSET(tty, &rfds)) {
      rc = term_pipe(tty, STDOUT_FILENO, NULL, NULL);
      if (rc) {
        break;
      }
    }

    if (FD_ISSET(in, &rfds)) {
      rc = term_pipe(in, tty, quit, &s);
      if (rc) {
        break;
      }
    }
  } while (quit[s] != 0);

  tcsetattr(in, TCSANOW, &otio);

out:
  return rc;
}

static void help(const char* name) {
  printf("Usage:\n %s -B 115200 /dev/ttyS0\n", name);
}

int main(int argc, char **argv) {
  const char *ttypath, *imgpath;
  int tty = -1, term = -1;
  int c, rv, rc, prot, patch;
  size_t size;
  speed_t speed = B115200;

  while ((c = getopt(argc, argv, "B:")) != -1) {
    switch (c) {
      case 'B':
        speed = tty_speed(atoi(optarg));
        if (speed == -1) {
          return -1;
        }
    }
  }

  if (optind >= argc) {
    help(argv[0]);
    goto out;
  }

  ttypath = argv[optind++];

  tty = open_tty(ttypath, speed);
  if (tty < 0) {
    perror(ttypath);
    goto out;
  }

  if (term) {
    rc = terminal(tty);
    if (rc && !(errno == EINTR)) {
      perror("terminal");
      goto out;
    }
  }

  rv = 0;

out:
  if (tty >= 0) {
    close(tty);
  }

  return rv;
}
