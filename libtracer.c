#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#include <fcntl.h>
#include <unistd.h>

#include <dlfcn.h>

static void initialize (void);
static int initialized;
static int fh;

#define SS do {
#define SE } while(0)

#define assert_perror(cond) SS if (!(cond)) { perror (# cond); exit (63); }; SE

#include <stdio.h>

static void xwrite (const void *data, int len)
{
  while (len)
    {
      int written = write (fh, data, len);
      
      if (written < 0 && errno == EAGAIN)
        continue;

      assert_perror (written > 0);
      
      data = (char *)data + written;
      len -= written;
    }
}

#define gen_int(i)	SS int v_ = (i); xwrite (&v_, sizeof(int)); SE
#define gen_char(d)	SS char v_ = (d); xwrite (&v_, 1); SE

static void gen_str(const char *s)
{
  int len = strlen (s);
  gen_int (len);
  xwrite (s, len);
}

static void gen_cwd (void)
{
  char cwd[PATH_MAX];

  assert_perror (getcwd (cwd, PATH_MAX) == cwd);

  gen_str (cwd);
}

static void gen_sync (void)
{
  char sync;
  int xread;

  gen_char ('S');

  do {
    xread = read (fh, &sync, 1);
    if (xread == -1 && errno == EAGAIN)
      continue;
    
    assert_perror (xread == 1);
  } while(xread != 1);

  assert_perror (sync == 's');
}

static void gen_change (const char *path)
{
  if (!initialized)
    initialize ();

  gen_char ('C');
  /* this is only an optimization */
  path[0] == '/' ? gen_str ("") : gen_cwd ();
  gen_str (path);
  /**/
  gen_sync ();
}

/* socket handling */

static void initialize (void)
{
  if (!initialized)
    {
      struct sockaddr_un sa;
      char *socket_path;

      assert_perror (socket_path = getenv ("INSTALLTRACER_SOCKET"));

      sa.sun_family = AF_UNIX;
      strncpy (sa.sun_path, socket_path, sizeof (sa.sun_path));

      assert_perror ((fh = socket (AF_UNIX, SOCK_STREAM, PF_UNSPEC)) >= 0);
      assert_perror (connect (fh, &sa, sizeof sa) >= 0);

      initialized = 1;

      gen_char ('I'); gen_int (getpid ()); gen_sync ();
    }
}

static void uninitialize (void)
{
  if (initialized)
    {
      close (fh);
      initialized = 0;
    }
}

/* stub functions following */

#define REAL_FUNC(res,name,proto) 		\
    static res (*real_func)proto;		\
    if (!real_func)				\
      real_func = dlsym (RTLD_NEXT, #name);	\
    assert_perror (real_func)

int open (const char *file, int oflag, ...)
{
  mode_t mode;
  REAL_FUNC (int,open,(const char *,int,mode_t));

  if (oflag & O_CREAT)
    {
      va_list arg;

      gen_change (file);

      va_start (arg, oflag);
      mode = va_arg(arg, mode_t);
      va_end(arg);
    }

  return real_func (file, oflag, mode);
}

int open64 (const char *file, int oflag, ...)
{
  mode_t mode;
  REAL_FUNC (int,open64,(const char *,int,mode_t));

  if (oflag & O_CREAT)
    {
      va_list arg;

      gen_change (file);

      va_start (arg, oflag);
      mode = va_arg(arg, mode_t);
      va_end(arg);
    }

  return real_func (file, oflag, mode);
}

int creat (const char *file, mode_t mode)
{
  REAL_FUNC (int,creat,(const char *,mode_t));
  gen_change (file);
  return real_func (file, mode);
}

int creat64 (const char *file, mode_t mode)
{
  REAL_FUNC (int,creat64,(const char *,mode_t));
  gen_change (file);
  return real_func (file, mode);
}

int mkdir (const char *path, mode_t mode)
{
  REAL_FUNC (int,mkdir,(const char *,mode_t));
  gen_change (path);
  return real_func (path, mode);
}

int rmdir (const char *path)
{
  REAL_FUNC (int,rmdir,(const char *));
  gen_change (path);
  return real_func (path);
}

int unlink (const char *path)
{
  REAL_FUNC (int,unlink,(const char *));
  gen_change (path);
  return real_func (path);
}

int remove (const char *path)
{
  REAL_FUNC (int,remove,(const char *));
  gen_change (path);
  return real_func (path);
}

int rename (const char *oldpath, const char *newpath)
{
  REAL_FUNC (int,rename,(const char *,const char *));
  gen_change (oldpath);
  gen_change (newpath);
  return real_func (oldpath, newpath);
}

int link (const char *oldpath, const char *newpath)
{
  REAL_FUNC (int,link,(const char *,const char *));
  gen_change (newpath);
  return real_func (oldpath, newpath);
}

int symlink (const char *oldpath, const char *newpath)
{
  REAL_FUNC (int,symlink,(const char *,const char *));
  gen_change (newpath);
  return real_func (oldpath, newpath);
}

/* vfork is not a problem, but fork might, so cut the connection */

pid_t fork (void)
{
  REAL_FUNC (pid_t,fork,(void));
  uninitialize ();
  return real_func ();
}
