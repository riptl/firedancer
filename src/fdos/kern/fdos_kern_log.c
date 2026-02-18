#define _POSIX_C_SOURCE 200809
#include "../../util/log/fd_log.h"
#include "../../util/cstr/fd_cstr.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* TEXT_* are quick-and-dirty color terminal hacks.  Probably should
   do something more robust longer term. */

#define TEXT_NORMAL    "\033[0m"
#define TEXT_BOLD      "\033[1m"
#define TEXT_UNDERLINE "\033[4m"
#define TEXT_BLINK     "\033[5m"

#define TEXT_BLUE      "\033[34m"
#define TEXT_GREEN     "\033[32m"
#define TEXT_YELLOW    "\033[93m"
#define TEXT_RED       "\033[31m"

/* APPLICATION LOGICAL ID APIS ****************************************/

/* App id */

static ulong fd_log_private_app_id; /* 0 outside boot/halt, init on boot */

void fd_log_private_app_id_set( ulong app_id ) { fd_log_private_app_id = app_id; }

ulong fd_log_app_id( void ) { return fd_log_private_app_id; }

/* App */

static char fd_log_private_app[ FD_LOG_NAME_MAX ]; /* "" outside boot/halt, init on boot */

void
fd_log_private_app_set( char const * app ) {
  if( FD_UNLIKELY( !app ) ) app = "[app]";
  if( FD_LIKELY( app!=fd_log_private_app ) )
    fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_app ), app, FD_LOG_NAME_MAX-1UL ) );
}

char const * fd_log_app( void ) { return fd_log_private_app; }

/* Thread ID */

static FD_TL ulong fd_log_private_thread_id;      /* 0 at thread start */
static FD_TL int   fd_log_private_thread_id_init; /* 0 at thread start */

void
fd_log_private_thread_id_set( ulong thread_id ) {
  fd_log_private_thread_id      = thread_id;
  fd_log_private_thread_id_init = 1;
}

ulong
fd_log_thread_id( void ) {
  return fd_log_private_thread_id;
}

/* Thread */

static char fd_log_private_thread[ FD_LOG_NAME_MAX ] = {"?"}; /* "" at thread start */

void
fd_log_thread_set( char const * thread ) {
  fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_thread ), thread, FD_LOG_NAME_MAX-1UL ) );
}

char const *
fd_log_thread( void ) {
  return fd_log_private_thread;
}

/* APPLICATION PHYSICAL ID APIS ***************************************/

/* Host ID */

static ulong fd_log_private_host_id; /* 0 outside boot/halt, initialized on boot */

void fd_log_private_host_id_set( ulong host_id ) { fd_log_private_host_id = host_id; }

ulong fd_log_host_id( void ) { return fd_log_private_host_id; }

/* Host */

static char  fd_log_private_host[ FD_LOG_NAME_MAX ]; /* "" outside boot/halt, initialized on boot */

char const * fd_log_host( void ) { return fd_log_private_host; }

void
fd_log_private_host_set( char const * host ) {
  if( FD_UNLIKELY( !host ) || FD_UNLIKELY( host[0]=='\0') ) host = "[host]";
  if( FD_LIKELY( host!=fd_log_private_host ) )
    fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_host ), host, FD_LOG_NAME_MAX-1UL ) );
}

/* CPU ID */

static FD_TL ulong fd_log_private_cpu_id; /* 0 at thread start */

void
fd_log_private_cpu_id_set( ulong cpu_id ) {
  fd_log_private_cpu_id = cpu_id;
}

ulong
fd_log_cpu_id( void ) {
  return fd_log_private_cpu_id;
}

/* CPU */

static FD_TL char fd_log_private_cpu[ FD_LOG_NAME_MAX ]; /* "" at thread start */
static FD_TL int  fd_log_private_cpu_init;               /* 0  at thread start */

void
fd_log_cpu_set( char const * cpu ) {
  if( FD_UNLIKELY( !cpu ) || FD_UNLIKELY( cpu[0]=='\0') ) {
    strcpy( fd_log_private_cpu, "?" );
    fd_log_private_cpu_init = 1;
  } else if( FD_LIKELY( cpu!=fd_log_private_cpu ) ) {
    fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_cpu ), cpu, FD_LOG_NAME_MAX-1UL ) );
    fd_log_private_cpu_init = 1;
  }
}

char const *
fd_log_cpu( void ) {
  if( FD_UNLIKELY( !fd_log_private_cpu_init ) ) fd_log_cpu_set( NULL );
  return fd_log_private_cpu;
}

/* THREAD GROUP ID APIS ***********************************************/

/* Group id */

static ulong fd_log_private_group_id; /* 0 outside boot/halt, init on boot */

void fd_log_private_group_id_set( ulong group_id ) { fd_log_private_group_id = group_id; }

ulong fd_log_group_id( void ) { return fd_log_private_group_id; }

/* Group */

static char fd_log_private_group[ FD_LOG_NAME_MAX ]; /* "" outside boot/halt, init on boot */

char const * fd_log_group( void ) { return fd_log_private_group; }

void
fd_log_private_group_set( char const * group ) {
  if( FD_UNLIKELY( !group ) || FD_UNLIKELY( group[0]=='\0') ) group = "[group]";
  if( FD_LIKELY( group!=fd_log_private_group ) )
    fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_group ), group, FD_LOG_NAME_MAX-1UL ) );
}

static FD_TL ulong fd_log_private_tid; /* 0 at thread start */

void
fd_log_private_tid_set( ulong tid ) {
  fd_log_private_tid = tid;
}

ulong
fd_log_tid( void ) {
  return fd_log_private_tid;
}

/* User id */

static ulong fd_log_private_user_id; /* 0 outside boot/halt, init on boot */

void
fd_log_private_user_id_set( ulong user_id ) {
  fd_log_private_user_id = user_id;
}

ulong
fd_log_user_id( void ) {
  return fd_log_private_user_id;
}

/* User */

static char  fd_log_private_user[ FD_LOG_NAME_MAX ]; /* "" outside boot/halt, init on boot */

char const * fd_log_user( void ) { return fd_log_private_user; }

void
fd_log_private_user_set( char const * user ) {
  if( FD_UNLIKELY( !user ) || FD_UNLIKELY( user[0]=='\0') ) user = "[user]";
  if( FD_LIKELY( user!=fd_log_private_user ) )
    fd_cstr_fini( fd_cstr_append_cstr_safe( fd_cstr_init( fd_log_private_user ), user, FD_LOG_NAME_MAX-1UL ) );
}

/* WALLCLOCK APIS *****************************************************/

static fd_clock_func_t fd_log_private_clock_func = NULL;
static void const *    fd_log_private_clock_args = NULL;

long
fd_log_wallclock( void ) {
  return fd_log_private_clock_func( fd_log_private_clock_args );
}

void
fd_log_wallclock_set( fd_clock_func_t clock,
                      void const *    args ) {
  fd_log_private_clock_func = clock;
  fd_log_private_clock_args = args;
}

char *
fd_log_wallclock_cstr( long   now,
                       char * buf ) {
  uint  YYYY;
  uint  MM;
  uint  DD;
  uint  hh;
  uint  mm;
  ulong ns;
  int   tz;

  static long const ns_per_m = 60000000000L;
  static long const ns_per_s =  1000000000L;

  static FD_TL long now_ref  = 1262325600000000000L; /* 2010-01-01 00:00:00.000000000 GMT-06 */
  static FD_TL uint YYYY_ref = 2010U;                /* Initialized to what now0 corresponds to */
  static FD_TL uint MM_ref   = 1U;                   /* " */
  static FD_TL uint DD_ref   = 1U;                   /* " */
  static FD_TL uint hh_ref   = 0U;                   /* " */
  static FD_TL uint mm_ref   = 0U;                   /* " */
  static FD_TL int  tz_ref   = -6;                   /* " */

  if( FD_LIKELY( (now_ref<=now) & (now<(now_ref+ns_per_m)) ) ) {

    /* now is near the reference timestamp so we reuse the reference
       calculation timestamp. */

    YYYY = YYYY_ref;
    MM   = MM_ref;
    DD   = DD_ref;
    hh   = hh_ref;
    mm   = mm_ref;
    ns   = (ulong)(now - now_ref);
    tz   = tz_ref;

  } else {

    long _t  = now / ns_per_s;
    long _ns = now - ns_per_s*_t;
    if( _ns<0L ) _ns += ns_per_s, _t--;
    time_t t = (time_t)_t;

    struct tm tm[1];
    static FD_TL int localtime_broken = 0;
    if( FD_UNLIKELY( !localtime_broken && !localtime_r( &t, tm ) ) ) localtime_broken = 1;
    if( FD_UNLIKELY( localtime_broken ) ) { /* If localtime_r doesn't work, pretty print as a raw UNIX time */
      /* Note: These can all run in parallel */
      fd_cstr_append_fxp10_as_text( buf,    ' ', fd_char_if( now<0L, '-', '\0' ), 9UL, fd_long_abs( now ), 29UL );
      fd_cstr_append_text         ( buf+29, " s UNIX",                                                      7UL );
      fd_cstr_append_char         ( buf+36, '\0'                                                                );
      return buf;
    }

    YYYY = (uint)(1900+tm->tm_year);
    MM   = (uint)(   1+tm->tm_mon );
    DD   = (uint)tm->tm_mday;
    hh   = (uint)tm->tm_hour;
    mm   = (uint)tm->tm_min;
    ns   = ((ulong)((uint)tm->tm_sec))*((ulong)ns_per_s) + ((ulong)_ns);
#   if defined(__linux__)
    tz   = (int)(-timezone/3600L+(long)tm->tm_isdst);
#   else
    tz   = 0;
#   endif

    now_ref  = now - (long)ns;
    YYYY_ref = YYYY;
    MM_ref   = MM;
    DD_ref   = DD;
    hh_ref   = hh;
    mm_ref   = mm;
    tz_ref   = tz;

  }

  /* Note: These can all run in parallel! */
  fd_cstr_append_uint_as_text ( buf,    '0', '\0',    YYYY,            4UL );
  fd_cstr_append_char         ( buf+ 4, '-'                                );
  fd_cstr_append_uint_as_text ( buf+ 5, '0', '\0',      MM,            2UL );
  fd_cstr_append_char         ( buf+ 7, '-'                                );
  fd_cstr_append_uint_as_text ( buf+ 8, '0', '\0',      DD,            2UL );
  fd_cstr_append_char         ( buf+10, ' '                                );
  fd_cstr_append_uint_as_text ( buf+11, '0', '\0',      hh,            2UL );
  fd_cstr_append_char         ( buf+13, ':'                                );
  fd_cstr_append_uint_as_text ( buf+14, '0', '\0',      mm,            2UL );
  fd_cstr_append_char         ( buf+16, ':'                                );
  fd_cstr_append_fxp10_as_text( buf+17, '0', '\0', 9UL, ns,           12UL );
  fd_cstr_append_text         ( buf+29, " GMT",                        4UL );
  fd_cstr_append_char         ( buf+33, fd_char_if( tz<0, '-', '+' )       );
  fd_cstr_append_uint_as_text ( buf+34, '0', '\0', fd_int_abs( tz ),   2UL );
  fd_cstr_append_char         ( buf+36, '\0'                               );
  return buf;
}

/* LOG APIS ***********************************************************/

char       fd_log_private_path[ 1024 ]; /* "" outside boot/halt, init at boot */
static int fd_log_private_dedup;        /*  0 outside boot/halt, init at boot */

static int fd_log_private_colorize;      /* 0 outside boot/halt, init at boot */
static int fd_log_private_level_logfile; /* 0 outside boot/halt, init at boot */
static int fd_log_private_level_stderr;  /* 0 outside boot/halt, init at boot */
static int fd_log_private_level_flush;   /* 0 outside boot/halt, init at boot */

int fd_log_colorize     ( void ) { return FD_VOLATILE_CONST( fd_log_private_colorize      ); }
int fd_log_level_logfile( void ) { return FD_VOLATILE_CONST( fd_log_private_level_logfile ); }
int fd_log_level_stderr ( void ) { return FD_VOLATILE_CONST( fd_log_private_level_stderr  ); }
int fd_log_level_flush  ( void ) { return FD_VOLATILE_CONST( fd_log_private_level_flush   ); }

void fd_log_colorize_set     ( int mode  ) { FD_VOLATILE( fd_log_private_colorize      ) = mode;  }
void fd_log_level_logfile_set( int level ) { FD_VOLATILE( fd_log_private_level_logfile ) = level; }
void fd_log_level_stderr_set ( int level ) { FD_VOLATILE( fd_log_private_level_stderr  ) = level; }
void fd_log_level_flush_set  ( int level ) { FD_VOLATILE( fd_log_private_level_flush   ) = level; }

/* Buffer size used for vsnprintf calls (this is also one more than the
   maximum size that this can passed to fd_io_write) */

#define FD_LOG_BUF_SZ (32UL*4096UL)

__attribute__((no_sanitize_address)) void
fd_log_private_fprintf_0( int          fd,
                          char const * fmt, ... ) {

  /* Note: while this function superficially looks vdprintf-ish, we don't
     use that as it can do all sorts of unpleasantness under the hood
     (fflush, mutex / futex on fd, non-AS-safe buffering, ...) that this
     function deliberately avoids.  Also, the function uses the shared
     lock to help keep messages generated from processes that share the
     same log fd sane. */

  /* TODO:
     - Consider moving to util/io as fd_io_printf or renaming to
       fd_log_printf?
     - Is msg better to have on stack or in thread local storage?
     - Is msg even necessary given shared lock? (probably still useful to
       keep the message write to be a single-system-call best effort)
     - Allow partial write to fd_io_write?  (e.g. src_min=0 such that
       the fd_io_write below is guaranteed to be a single system call) */

  char msg[ FD_LOG_BUF_SZ ];

  va_list ap;
  va_start( ap, fmt );
  int len = vsnprintf( msg, FD_LOG_BUF_SZ, fmt, ap );
  if( len<0                        ) len = 0;                        /* cmov */
  if( len>(int)(FD_LOG_BUF_SZ-1UL) ) len = (int)(FD_LOG_BUF_SZ-1UL); /* cmov */
  msg[ len ] = '\0';
  va_end( ap );

  ulong wsz;
  fd_io_write( fd, msg, (ulong)len, (ulong)len, &wsz ); /* Note: we ignore errors because what are we doing to do? log them? */
}

/* This is the same as fd_log_private_fprintf_0 except that it does not try to
   take a lock when writing to the log file.  This should almost never be used
   except in exceptional cases when logging while the process is shutting down.

   It exists because if a child process dies while holding the lock, we may
   want to log some diagnostic messages when tearing down the process tree. */
void
fd_log_private_fprintf_nolock_0( int          fd,
                                 char const * fmt, ... ) {

  /* Note: while this function superficially looks vdprintf-ish, we don't
     use that as it can do all sorts of unpleasantness under the hood
     (fflush, mutex / futex on fd, non-AS-safe buffering, ...) that this
     function deliberately avoids.  Also, the function uses the shared
     lock to help keep messages generated from processes that share the
     same log fd sane. */

  /* TODO:
     - Consider moving to util/io as fd_io_printf or renaming to
       fd_log_printf?
     - Is msg better to have on stack or in thread local storage?
     - Is msg even necessary given shared lock? (probably still useful to
       keep the message write to be a single-system-call best effort)
     - Allow partial write to fd_io_write?  (e.g. src_min=0 such that
       the fd_io_write below is guaranteed to be a single system call) */

  char msg[ FD_LOG_BUF_SZ ];

  va_list ap;
  va_start( ap, fmt );
  int len = vsnprintf( msg, FD_LOG_BUF_SZ, fmt, ap );
  if( len<0                        ) len = 0;                        /* cmov */
  if( len>(int)(FD_LOG_BUF_SZ-1UL) ) len = (int)(FD_LOG_BUF_SZ-1UL); /* cmov */
  msg[ len ] = '\0';
  va_end( ap );

  ulong wsz;
  fd_io_write( fd, msg, (ulong)len, (ulong)len, &wsz ); /* Note: we ignore errors because what are we doing to do? log them? */
}

/* Log buffer used by fd_log_private_0 and fd_log_private_hexdump_msg */

static FD_TL char fd_log_private_log_msg[ FD_LOG_BUF_SZ ];

char const *
fd_log_private_0( char const * fmt, ... ) {
  va_list ap;
  va_start( ap, fmt );
  int len = vsnprintf( fd_log_private_log_msg, FD_LOG_BUF_SZ, fmt, ap );
  if( len<0                        ) len = 0;                        /* cmov */
  if( len>(int)(FD_LOG_BUF_SZ-1UL) ) len = (int)(FD_LOG_BUF_SZ-1UL); /* cmov */
  fd_log_private_log_msg[ len ] = '\0';
  va_end( ap );
  return fd_log_private_log_msg;
}

char const *
fd_log_private_hexdump_msg( char const * descr,
                            void const * mem,
                            ulong        sz ) {

# define FD_LOG_HEXDUMP_BYTES_PER_LINE           (16UL)
# define FD_LOG_HEXDUMP_BLOB_DESCRIPTION_MAX_LEN (32UL)
# define FD_LOG_HEXDUMP_MAX_INPUT_BLOB_SZ        (1664UL) /* multiple of 128 >= 1542 */

# define FD_LOG_HEXDUMP_ADD_TO_LOG_BUF(...)  do { log_buf_ptr += fd_int_max( sprintf( log_buf_ptr, __VA_ARGS__ ), 0 ); } while(0)
  char * log_buf_ptr = fd_log_private_log_msg; /* used by FD_LOG_HEXDUMP_ADD_TO_LOG_BUF macro */

  /* Print the hexdump header */
  /* FIXME: consider additional sanitization of descr or using compiler
     tricks to prevent user from passing a non-const-char string (i.e.
     data they got from somewhere else that might not be sanitized). */

  if( FD_UNLIKELY( !descr ) ) {

    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "HEXDUMP - (%lu bytes at 0x%lx)", sz, (ulong)mem );

  } else if( FD_UNLIKELY( strlen( descr )>FD_LOG_HEXDUMP_BLOB_DESCRIPTION_MAX_LEN ) ) {

    char tmp[ FD_LOG_HEXDUMP_BLOB_DESCRIPTION_MAX_LEN + 1UL ];
    fd_cstr_fini( fd_cstr_append_text( fd_cstr_init( tmp ), descr, FD_LOG_HEXDUMP_BLOB_DESCRIPTION_MAX_LEN ) );
    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "HEXDUMP \"%s\"... (%lu bytes at 0x%lx)", tmp, sz, (ulong)mem );

  } else {

    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "HEXDUMP \"%s\" (%lu bytes at 0x%lx)", descr, sz, (ulong)mem );

  }

  if( FD_UNLIKELY( !sz ) ) return fd_log_private_log_msg;

  FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "\n" );

  if( FD_UNLIKELY( !mem ) ) {
    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "\t... snip (unreadable memory) ..." );
    return fd_log_private_log_msg;
  }

  char         line_buf[ FD_LOG_HEXDUMP_BYTES_PER_LINE+1 ];
  char const * blob     = (char const *)mem;
  ulong        blob_off = 0UL;
  ulong        blob_sz  = fd_ulong_min( sz, FD_LOG_HEXDUMP_MAX_INPUT_BLOB_SZ );

  for( ; blob_off<blob_sz; blob_off++ ) {
    ulong col_idx = blob_off % FD_LOG_HEXDUMP_BYTES_PER_LINE;

    /* New line. Print previous line's ASCII representation and then print the offset. */
    if( FD_UNLIKELY( !col_idx ) ) {
      if( FD_LIKELY( blob_off ) ) FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "  %s\n", line_buf );
      FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "\t%04lx: ", blob_off );
    }
    /* FIXME: consider extra space between col 7 and 8 to make easier
       for visual inspection */

    char c = blob[blob_off];
    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( " %02x", (uint)(uchar)c );

    /* If not a printable ASCII character, output a dot. */
    line_buf[ col_idx     ] = fd_char_if( fd_isalnum( (int)c ) | fd_ispunct( (int)c ) | (c==' '), c, '.' );
    line_buf[ col_idx+1UL ] = '\0';
  }

  /* Print the 2nd column of last blob line */
  while( blob_off % FD_LOG_HEXDUMP_BYTES_PER_LINE ) {
    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "   " );
    blob_off++;
  }
  FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "  %s", line_buf );

  if( FD_UNLIKELY( blob_sz < sz ) )
    FD_LOG_HEXDUMP_ADD_TO_LOG_BUF( "\n\t... snip (printed %lu bytes, omitted %lu bytes) ...", blob_sz, sz-blob_sz );

  return fd_log_private_log_msg;

# undef FD_LOG_HEXDUMP_BYTES_PER_LINE
# undef FD_LOG_HEXDUMP_BLOB_DESCRIPTION_MAX_LEN
# undef FD_LOG_HEXDUMP_MAX_INPUT_BLOB_SZ
# undef FD_LOG_HEXDUMP_ADD_TO_LOG_BUF
}

void
fd_log_private_1( int          level,
                  long         now,
                  char const * file,
                  int          line,
                  char const * func,
                  char const * msg ) {

  if( level<fd_log_level_logfile() ) return;

  /* These are thread init so we call them regardless of permanent log
     enabled to their initialization time is guaranteed independent of
     whether the permanent log is enabled. */

  char const * thread = fd_log_thread();
  char const * cpu    = fd_log_cpu();
  ulong        tid    = fd_log_tid();

  int log_fileno = 3;
  int to_logfile = (log_fileno!=-1);
  int to_stderr  = (level>=fd_log_level_stderr());
  if( !(to_logfile | to_stderr) ) return;

  /* Deduplicate the log if requested */

  if( fd_log_private_dedup ) {

    /* Compute if this message appears to be a recent duplicate of
       a previous log message */

    ulong hash = fd_cstr_hash_append( fd_cstr_hash_append( fd_cstr_hash_append( fd_ulong_hash(
                   (ulong)(8L*(long)line+(long)level) ), file ), func ), msg );

    static long const dedup_interval = 20000000L; /* 1/50 s */

    static FD_TL int   init;      /* 0   on thread start */
    static FD_TL ulong last_hash; /* 0UL on thread start */
    static FD_TL long  then;      /* 0L  on thread start */

    int is_dup = init & (hash==last_hash) & ((now-then)<dedup_interval);
    init = 1;

    /* Update how many messages from this thread in row have been
       duplicates */

    static FD_TL ulong dedup_cnt;   /* 0UL on thread start */
    static FD_TL int   in_dedup;    /* 0   on thread start */

    if( is_dup ) dedup_cnt++;
    else {
      if( in_dedup ) {

        /* This message appears to end a long string of duplicates.
           Log the end of the deduplication. */

        char then_cstr[ FD_LOG_WALLCLOCK_CSTR_BUF_SZ ];
        fd_log_wallclock_cstr( then, then_cstr );

        if( to_logfile )
          fd_log_private_fprintf_0( 3, "SNIP    %s %6lu:%-6lu %s:%s:%-4s %s:%s:%-4s "
                                    "stopped repeating (%lu identical messages)\n",
                                    then_cstr, fd_log_group_id(),tid, fd_log_user(),fd_log_host(),cpu,
                                    fd_log_app(),fd_log_group(),thread, dedup_cnt+1UL );

        if( to_stderr ) {
          char * then_short_cstr = then_cstr+5; then_short_cstr[21] = '\0'; /* Lop off the year, ns resolution and timezone */
          fd_log_private_fprintf_0( 2, "SNIP    %s %-6lu %-4s %-4s stopped repeating (%lu identical messages)\n",
                                    then_short_cstr, tid,cpu,thread, dedup_cnt+1UL );
        }

        in_dedup = 0;
      }

      dedup_cnt = 0UL;
    }

    /* dedup_cnt previous messages from this thread appear to be
       duplicates.  Decide whether to let the raw message print or
       deduplicate to the log.  FIXME: CONSIDER RANDOMIZING THE
       THROTTLE. */

    static ulong const dedup_thresh   = 3UL;         /* let initial dedup_thresh duplicates go out the door */
    static long  const dedup_throttle = 1000000000L; /* ~1s, how often to update status on current duplication */

    static FD_TL long dedup_last; /* 0L on thread start */

    if( dedup_cnt < dedup_thresh ) dedup_last = now;
    else {
      if( (now-dedup_last) >= dedup_throttle ) {
        char now_cstr[ FD_LOG_WALLCLOCK_CSTR_BUF_SZ ];
        fd_log_wallclock_cstr( now, now_cstr );
        if( to_logfile )
          fd_log_private_fprintf_0( 3, "SNIP    %s %6lu:%-6lu %s:%s:%-4s %s:%s:%-4s repeating (%lu identical messages)\n",
                                    now_cstr, fd_log_group_id(),tid, fd_log_user(),fd_log_host(),cpu,
                                    fd_log_app(),fd_log_group(),thread, dedup_cnt+1UL );
        if( to_stderr ) {
          char * now_short_cstr = now_cstr+5; now_short_cstr[21] = '\0'; /* Lop off the year, ns resolution and timezone */
          fd_log_private_fprintf_0( 2, "SNIP    %s %-6lu %-4s %-4s repeating (%lu identical messages)\n",
                                    now_short_cstr, tid,cpu,thread, dedup_cnt+1UL );
        }
        dedup_last = now;
      }
      in_dedup = 1;
    }

    last_hash = hash;
    then      = now;

    if( in_dedup ) return;
  }

  char now_cstr[ FD_LOG_WALLCLOCK_CSTR_BUF_SZ ];
  fd_log_wallclock_cstr( now, now_cstr );

  static char const * level_cstr[] = {
    /* 0 */ "DEBUG  ",
    /* 1 */ "INFO   ",
    /* 2 */ "NOTICE ",
    /* 3 */ "WARNING",
    /* 4 */ "ERR    ",
    /* 5 */ "CRIT   ",
    /* 6 */ "ALERT  ",
    /* 7 */ "EMERG  "
  };

  if( to_logfile )
    fd_log_private_fprintf_0( log_fileno, "%s %s %6lu:%-6lu %s:%s:%-4s %s:%s:%-4s %s(%i)[%s]: %s\n",
                              level_cstr[level], now_cstr, fd_log_group_id(),tid, fd_log_user(),fd_log_host(),cpu,
                              fd_log_app(),fd_log_group(),thread, file,line,func, msg );

  if( to_stderr ) {
    static char const * color_level_cstr[] = {
      /* 0 */ TEXT_NORMAL                                  "DEBUG  ",
      /* 1 */ TEXT_BLUE                                    "INFO   " TEXT_NORMAL,
      /* 2 */ TEXT_GREEN                                   "NOTICE " TEXT_NORMAL,
      /* 3 */ TEXT_YELLOW                                  "WARNING" TEXT_NORMAL,
      /* 4 */ TEXT_RED                                     "ERR    " TEXT_NORMAL,
      /* 5 */ TEXT_RED TEXT_BOLD                           "CRIT   " TEXT_NORMAL,
      /* 6 */ TEXT_RED TEXT_BOLD TEXT_UNDERLINE            "ALERT  " TEXT_NORMAL,
      /* 7 */ TEXT_RED TEXT_BOLD TEXT_UNDERLINE TEXT_BLINK "EMERG  " TEXT_NORMAL
    };
    char * now_short_cstr = now_cstr+5; now_short_cstr[21] = '\0'; /* Lop off the year, ns resolution and timezone */
    fd_log_private_fprintf_0( 2, "%s %s %-6lu %-4s %-4s %s(%i): %s\n",
                              fd_log_private_colorize ? color_level_cstr[level] : level_cstr[level],
                              now_short_cstr, tid,cpu,thread, file, line, msg );
  }

  if( level<fd_log_level_flush() ) return;

  fd_log_flush();
}

void
fd_log_private_2( int          level,
                  long         now,
                  char const * file,
                  int          line,
                  char const * func,
                  char const * msg ) {
  fd_log_private_1( level, now, file, line, func, msg );
  __asm__ volatile ( "hlt" );
  __builtin_unreachable();
}
