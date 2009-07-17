/**********************************************************************

  random.c -

  $Author$
  created at: Fri Dec 24 16:39:21 JST 1993

  Copyright (C) 1993-2007 Yukihiro Matsumoto

**********************************************************************/

/*
This is based on trimmed version of MT19937.  To get the original version,
contact <http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html>.

The original copyright notice follows.

   A C-program for MT19937, with initialization improved 2002/2/10.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   This is a faster version by taking Shawn Cokus's optimization,
   Matthe Bellew's simplification, Isaku Wada's real version.

   Before using, initialize the state by using init_genrand(mt, seed)
   or init_by_array(mt, init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

#include <limits.h>
typedef int int_must_be_32bit_at_least[sizeof(int) * CHAR_BIT < 32 ? -1 : 1];

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfU	/* constant vector a */
#define UMASK 0x80000000U	/* most significant w-r bits */
#define LMASK 0x7fffffffU	/* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1U ? MATRIX_A : 0U))

enum {MT_MAX_STATE = N};

struct MT {
    /* assume int is enough to store 32bits */
    unsigned int state[N]; /* the array for the state vector  */
    unsigned int *next;
    int left;
};

#define genrand_initialized(mt) ((mt)->next != 0)
#define uninit_genrand(mt) ((mt)->next = 0)

/* initializes state[N] with a seed */
static void
init_genrand(struct MT *mt, unsigned int s)
{
    int j;
    mt->state[0] = s & 0xffffffffU;
    for (j=1; j<N; j++) {
        mt->state[j] = (1812433253U * (mt->state[j-1] ^ (mt->state[j-1] >> 30)) + j);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array state[].                     */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt->state[j] &= 0xffffffff;  /* for >32 bit machines */
    }
    mt->left = 1;
    mt->next = mt->state + N - 1;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
static void
init_by_array(struct MT *mt, unsigned int init_key[], int key_length)
{
    int i, j, k;
    init_genrand(mt, 19650218U);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt->state[i] = (mt->state[i] ^ ((mt->state[i-1] ^ (mt->state[i-1] >> 30)) * 1664525U))
          + init_key[j] + j; /* non linear */
        mt->state[i] &= 0xffffffffU; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt->state[0] = mt->state[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt->state[i] = (mt->state[i] ^ ((mt->state[i-1] ^ (mt->state[i-1] >> 30)) * 1566083941U))
          - i; /* non linear */
        mt->state[i] &= 0xffffffffU; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt->state[0] = mt->state[N-1]; i=1; }
    }

    mt->state[0] = 0x80000000U; /* MSB is 1; assuring non-zero initial array */
}

static void
next_state(struct MT *mt)
{
    unsigned int *p = mt->state;
    int j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    if (!genrand_initialized(mt)) init_genrand(mt, 5489U);

    mt->left = N;
    mt->next = mt->state;

    for (j=N-M+1; --j; p++)
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j=M; --j; p++)
        *p = p[M-N] ^ TWIST(p[0], p[1]);

    *p = p[M-N] ^ TWIST(p[0], mt->state[0]);
}

/* generates a random number on [0,0xffffffff]-interval */
static unsigned int
genrand_int32(struct MT *mt)
{
    unsigned int y;

    if (--mt->left <= 0) next_state(mt);
    y = *mt->next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,1) with 53-bit resolution*/
static double
genrand_real(struct MT *mt)
{
    unsigned int a = genrand_int32(mt)>>5, b = genrand_int32(mt)>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0);
}
/* These real versions are due to Isaku Wada, 2002/01/09 added */

#undef N
#undef M

/* These real versions are due to Isaku Wada, 2002/01/09 added */

#include "ruby/ruby.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

typedef struct {
    VALUE seed;
    struct MT mt;
} rb_random_t;

#define DEFAULT_SEED_CNT 4

struct Random {
    rb_random_t rnd;
    unsigned int initial[DEFAULT_SEED_CNT];
};

static struct Random default_rand;

unsigned long
rb_genrand_int32(void)
{
    return genrand_int32(&default_rand.rnd.mt);
}

double
rb_genrand_real(void)
{
    return genrand_real(&default_rand.rnd.mt);
}

#define BDIGITS(x) (RBIGNUM_DIGITS(x))
#define BITSPERDIG (SIZEOF_BDIGITS*CHAR_BIT)
#define BIGRAD ((BDIGIT_DBL)1 << BITSPERDIG)
#define DIGSPERINT (SIZEOF_INT/SIZEOF_BDIGITS)
#define BIGUP(x) ((BDIGIT_DBL)(x) << BITSPERDIG)
#define BIGDN(x) RSHIFT(x,BITSPERDIG)
#define BIGLO(x) ((BDIGIT)((x) & (BIGRAD-1)))
#define BDIGMAX ((BDIGIT)-1)

#define roomof(n, m) (int)(((n)+(m)-1) / (m))
#define numberof(array) (int)(sizeof(array) / sizeof((array)[0]))
#define SIZEOF_INT32 (31/CHAR_BIT + 1)

VALUE rb_cRandom;
#define id_minus '-'
#define id_plus  '+'

static VALUE random_seed(void);

/* :nodoc: */
static void
random_mark(void *ptr)
{
    rb_gc_mark(((rb_random_t *)ptr)->seed);
}

#define random_free RUBY_TYPED_DEFAULT_FREE

static size_t
random_memsize(void *ptr)
{
    return ptr ? sizeof(rb_random_t) : 0;
}

static const rb_data_type_t random_data_type = {
    "random",
    random_mark,
    random_free,
    random_memsize,
};

static rb_random_t *
get_rnd(VALUE obj)
{
    rb_random_t *ptr;
    TypedData_Get_Struct(obj, rb_random_t, &random_data_type, ptr);
    return ptr;
}

/* :nodoc: */
static VALUE
random_alloc(VALUE klass)
{
    rb_random_t *rnd;
    VALUE obj = TypedData_Make_Struct(rb_cRandom, rb_random_t, &random_data_type, rnd);
    rnd->seed = INT2FIX(0);
    return obj;
}

static VALUE
rand_init(struct MT *mt, VALUE vseed)
{
    volatile VALUE seed;
    long blen = 0;
    int i, j, len;
    unsigned int buf0[SIZEOF_LONG / SIZEOF_INT32 * 4], *buf = buf0;

    seed = rb_to_int(vseed);
    switch (TYPE(seed)) {
      case T_FIXNUM:
	len = 1;
	buf[0] = (unsigned int)(FIX2ULONG(seed) & 0xffffffff);
#if SIZEOF_LONG > SIZEOF_INT32
	if ((buf[1] = (unsigned int)(FIX2ULONG(seed) >> 32)) != 0) ++len;
#endif
	break;
      case T_BIGNUM:
	blen = RBIGNUM_LEN(seed);
	if (blen == 0) {
	    len = 1;
	}
	else if (blen > MT_MAX_STATE * SIZEOF_INT32 / SIZEOF_BDIGITS) {
	    blen = (len = MT_MAX_STATE) * SIZEOF_INT32 / SIZEOF_BDIGITS;
	    len = roomof(len, SIZEOF_INT32);
	}
	else {
	    len = roomof((int)blen * SIZEOF_BDIGITS, SIZEOF_INT32);
	}
	/* allocate ints for init_by_array */
	if (len > numberof(buf0)) buf = ALLOC_N(unsigned int, len);
	memset(buf, 0, len * sizeof(*buf));
	len = 0;
	for (i = (int)(blen-1); 0 <= i; i--) {
	    j = i * SIZEOF_BDIGITS / SIZEOF_INT32;
#if SIZEOF_BDIGITS < SIZEOF_INT32
	    buf[j] <<= SIZEOF_BDIGITS * CHAR_BIT;
#endif
	    buf[j] |= RBIGNUM_DIGITS(seed)[i];
	    if (!len && buf[j]) len = j;
	}
	++len;
	break;
      default:
	rb_raise(rb_eTypeError, "failed to convert %s into Integer",
		 rb_obj_classname(vseed));
    }
    if (len <= 1) {
        init_genrand(mt, buf[0]);
    }
    else {
        if (buf[len-1] == 1) /* remove leading-zero-guard */
            len--;
        init_by_array(mt, buf, len);
    }
    if (buf != buf0) xfree(buf);
    return seed;
}

/*
 * call-seq: Random.new([seed]) -> prng
 *
 * Creates new Mersenne Twister based pseudorandom number generator with
 * seed.  When the argument seed is omitted, the generator is initialized
 * with Random.seed.
 *
 * The argument seed is used to ensure repeatable sequences of random numbers
 * between different runs of the program.
 *
 *     prng = Random.new(1234)
 *     [ prng.rand, prng.rand ]   #=> [0.191519450378892, 0.622108771039832]
 *     [ prng.integer(10), prng.integer(1000) ]  #=> [4, 664]
 *     prng = Random.new(1234)
 *     [ prng.rand, prng.rand ]   #=> [0.191519450378892, 0.622108771039832]
 */
static VALUE
random_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE vseed;
    rb_random_t *rnd = get_rnd(obj);

    if (argc == 0) {
	vseed = random_seed();
    }
    else {
	rb_scan_args(argc, argv, "01", &vseed);
    }
    rnd->seed = rand_init(&rnd->mt, vseed);
    return obj;
}

#define DEFAULT_SEED_LEN (DEFAULT_SEED_CNT * sizeof(int))

#if defined(S_ISCHR) && !defined(DOSISH)
# define USE_DEV_URANDOM 1
#else
# define USE_DEV_URANDOM 0
#endif

static void
fill_random_seed(unsigned int seed[DEFAULT_SEED_CNT])
{
    static int n = 0;
    struct timeval tv;
#if USE_DEV_URANDOM
    int fd;
    struct stat statbuf;
#endif

    memset(seed, 0, DEFAULT_SEED_LEN);

#if USE_DEV_URANDOM
    if ((fd = open("/dev/urandom", O_RDONLY
#ifdef O_NONBLOCK
            |O_NONBLOCK
#endif
#ifdef O_NOCTTY
            |O_NOCTTY
#endif
#ifdef O_NOFOLLOW
            |O_NOFOLLOW
#endif
            )) >= 0) {
        if (fstat(fd, &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
            (void)read(fd, seed, DEFAULT_SEED_LEN);
        }
        close(fd);
    }
#endif

    gettimeofday(&tv, 0);
    seed[0] ^= tv.tv_usec;
    seed[1] ^= (unsigned int)tv.tv_sec;
#if SIZEOF_TIME_T > SIZEOF_INT
    seed[0] ^= (unsigned int)((time_t)tv.tv_sec >> SIZEOF_INT * CHAR_BIT);
#endif
    seed[2] ^= getpid() ^ (n++ << 16);
    seed[3] ^= (unsigned int)(VALUE)&seed;
#if SIZEOF_VOIDP > SIZEOF_INT
    seed[2] ^= (unsigned int)((VALUE)&seed >> SIZEOF_INT * CHAR_BIT);
#endif
}

static VALUE
make_seed_value(const void *ptr)
{
    BDIGIT *digits;
    NEWOBJ(big, struct RBignum);
    OBJSETUP(big, rb_cBignum, T_BIGNUM);

    RBIGNUM_SET_SIGN(big, 1);
    rb_big_resize((VALUE)big, DEFAULT_SEED_LEN / SIZEOF_BDIGITS + 1);
    digits = RBIGNUM_DIGITS(big);

    MEMCPY(digits, ptr, char, DEFAULT_SEED_LEN);

    /* set leading-zero-guard if need. */
    digits[RBIGNUM_LEN(big)-1] = digits[RBIGNUM_LEN(big)-2] <= 1 ? 1 : 0;

    return rb_big_norm((VALUE)big);
}

/*
 * call-seq: Random.seed -> integer
 *
 * Returns arbitrary value for seed.
 */
static VALUE
random_seed(void)
{
    unsigned int buf[DEFAULT_SEED_CNT];
    fill_random_seed(buf);
    return make_seed_value(buf);
}

/*
 * call-seq: prng.seed -> integer
 *
 * Returns the seed of the generator.
 */
static VALUE
random_get_seed(VALUE obj)
{
    return get_rnd(obj)->seed;
}

/* :nodoc: */
static VALUE
random_copy(VALUE obj, VALUE orig)
{
    rb_random_t *rnd1 = get_rnd(obj);
    rb_random_t *rnd2 = get_rnd(orig);
    struct MT *mt = &rnd1->mt;

    *rnd1 = *rnd2;
    mt->next = mt->state + numberof(mt->state) - mt->left + 1;
    return obj;
}

static VALUE
mt_state(const struct MT *mt)
{
    VALUE bigo = rb_big_new(sizeof(mt->state) / sizeof(BDIGIT), 1);
    BDIGIT *d = RBIGNUM_DIGITS(bigo);
    int i;

    for (i = 0; i < numberof(mt->state); ++i) {
	unsigned int x = mt->state[i];
#if SIZEOF_BDIGITS < SIZEOF_INT32
	int j;
	for (j = 0; j < SIZEOF_INT32 / SIZEOF_BDIGITS; ++j) {
	    *d++ = BIGLO(x);
	    x = BIGDN(x);
	}
#else
	*d++ = (BDIGIT)x;
#endif
    }
    return rb_big_norm(bigo);
}

static VALUE
random_state(VALUE obj)
{
    rb_random_t *rnd = get_rnd(obj);
    return mt_state(&rnd->mt);
}

static VALUE
random_s_state(VALUE klass)
{
    return mt_state(&default_rand.rnd.mt);
}

static VALUE
random_left(VALUE obj)
{
    rb_random_t *rnd = get_rnd(obj);
    return INT2FIX(rnd->mt.left);
}

static VALUE
random_s_left(VALUE klass)
{
    return INT2FIX(default_rand.rnd.mt.left);
}

/* :nodoc: */
static VALUE
random_dump(VALUE obj)
{
    rb_random_t *rnd = get_rnd(obj);
    VALUE dump = rb_ary_new2(3);

    rb_ary_push(dump, mt_state(&rnd->mt));
    rb_ary_push(dump, INT2FIX(rnd->mt.left));
    rb_ary_push(dump, rnd->seed);

    return dump;
}

/* :nodoc: */
static VALUE
random_load(VALUE obj, VALUE dump)
{
    rb_random_t *rnd = get_rnd(obj);
    struct MT *mt = &rnd->mt;
    VALUE state, left = INT2FIX(1), seed = INT2FIX(0);
    VALUE *ary;
    unsigned long x;

    Check_Type(dump, T_ARRAY);
    ary = RARRAY_PTR(dump);
    switch (RARRAY_LEN(dump)) {
      case 3:
	seed = ary[2];
      case 2:
	left = ary[1];
      case 1:
	state = ary[0];
	break;
      default:
	rb_raise(rb_eArgError, "wrong dump data");
    }
    memset(mt->state, 0, sizeof(mt->state));
    if (FIXNUM_P(state)) {
	x = FIX2ULONG(state);
	mt->state[0] = (unsigned int)x;
#if SIZEOF_LONG / SIZEOF_INT >= 2
	mt->state[1] = (unsigned int)(x >> CHAR_BIT * SIZEOF_BDIGITS);
#endif
#if SIZEOF_LONG / SIZEOF_INT >= 3
	mt->state[2] = (unsigned int)(x >> 2 * CHAR_BIT * SIZEOF_BDIGITS);
#endif
#if SIZEOF_LONG / SIZEOF_INT >= 4
	mt->state[3] = (unsigned int)(x >> 3 * CHAR_BIT * SIZEOF_BDIGITS);
#endif
    }
    else {
	BDIGIT *d;
	long len;
	Check_Type(state, T_BIGNUM);
	len = RBIGNUM_LEN(state);
	if (len > roomof(sizeof(mt->state), SIZEOF_BDIGITS)) {
	    len = roomof(sizeof(mt->state), SIZEOF_BDIGITS);
	}
#if SIZEOF_BDIGITS < SIZEOF_INT
	else if (len % DIGSPERINT) {
	    d = RBIGNUM_DIGITS(state) + len;
# if DIGSPERINT == 2
	    --len;
	    x = *--d;
# else
	    x = 0;
	    do {
		x = (x << CHAR_BIT * SIZEOF_BDIGITS) | *--d;
	    } while (--len % DIGSPERINT);
# endif
	    mt->state[len / DIGSPERINT] = (unsigned int)x;
	}
#endif
	if (len > 0) {
	    d = BDIGITS(state) + len;
	    do {
		--len;
		x = *--d;
# if DIGSPERINT == 2
		--len;
		x = (x << CHAR_BIT * SIZEOF_BDIGITS) | *--d;
# elif SIZEOF_BDIGITS < SIZEOF_INT
		do {
		    x = (x << CHAR_BIT * SIZEOF_BDIGITS) | *--d;
		} while (--len % DIGSPERINT);
# endif
		mt->state[len / DIGSPERINT] = (unsigned int)x;
	    } while (len > 0);
	}
    }
    x = NUM2ULONG(left);
    if (x > numberof(mt->state)) {
	rb_raise(rb_eArgError, "wrong value");
    }
    mt->left = (unsigned int)x;
    mt->next = mt->state + numberof(mt->state) - x + 1;
    rnd->seed = rb_to_int(seed);

    return obj;
}

/*
 *  call-seq:
 *     srand(number=0)    => old_seed
 *
 *  Seeds the pseudorandom number generator to the value of
 *  <i>number</i>. If <i>number</i> is omitted
 *  or zero, seeds the generator using a combination of the time, the
 *  process id, and a sequence number. (This is also the behavior if
 *  <code>Kernel::rand</code> is called without previously calling
 *  <code>srand</code>, but without the sequence.) By setting the seed
 *  to a known value, scripts can be made deterministic during testing.
 *  The previous seed value is returned. Also see <code>Kernel::rand</code>.
 */

static VALUE
rb_f_srand(int argc, VALUE *argv, VALUE obj)
{
    VALUE seed, old;

    rb_secure(4);
    if (argc == 0) {
	seed = random_seed();
    }
    else {
	rb_scan_args(argc, argv, "01", &seed);
    }
    old = default_rand.rnd.seed;
    default_rand.rnd.seed = rand_init(&default_rand.rnd.mt, seed);

    return old;
}

static unsigned long
make_mask(unsigned long x)
{
    x = x | x >> 1;
    x = x | x >> 2;
    x = x | x >> 4;
    x = x | x >> 8;
    x = x | x >> 16;
#if 4 < SIZEOF_LONG
    x = x | x >> 32;
#endif
    return x;
}

static unsigned long
limited_rand(struct MT *mt, unsigned long limit)
{
    unsigned long mask = make_mask(limit);
    int i;
    unsigned long val;

  retry:
    val = 0;
    for (i = SIZEOF_LONG/SIZEOF_INT32-1; 0 <= i; i--) {
        if ((mask >> (i * 32)) & 0xffffffff) {
            val |= (unsigned long)genrand_int32(mt) << (i * 32);
            val &= mask;
            if (limit < val)
                goto retry;
        }
    }
    return val;
}

static VALUE
limited_big_rand(struct MT *mt, struct RBignum *limit)
{
    unsigned long mask, lim, rnd;
    struct RBignum *val;
    long i, len;
    int boundary;

    len = (RBIGNUM_LEN(limit) * SIZEOF_BDIGITS + 3) / 4;
    val = (struct RBignum *)rb_big_clone((VALUE)limit);
    RBIGNUM_SET_SIGN(val, 1);
#if SIZEOF_BDIGITS == 2
# define BIG_GET32(big,i) \
    (RBIGNUM_DIGITS(big)[(i)*2] | \
     ((i)*2+1 < RBIGNUM_LEN(big) ? \
      (RBIGNUM_DIGITS(big)[(i)*2+1] << 16) : \
      0))
# define BIG_SET32(big,i,d) \
    ((RBIGNUM_DIGITS(big)[(i)*2] = (d) & 0xffff), \
     ((i)*2+1 < RBIGNUM_LEN(big) ? \
      (RBIGNUM_DIGITS(big)[(i)*2+1] = (d) >> 16) : \
      0))
#else
    /* SIZEOF_BDIGITS == 4 */
# define BIG_GET32(big,i) (RBIGNUM_DIGITS(big)[i])
# define BIG_SET32(big,i,d) (RBIGNUM_DIGITS(big)[i] = (d))
#endif
  retry:
    mask = 0;
    boundary = 1;
    for (i = len-1; 0 <= i; i--) {
        lim = BIG_GET32(limit, i);
        mask = mask ? 0xffffffff : make_mask(lim);
        if (mask) {
            rnd = genrand_int32(mt) & mask;
            if (boundary) {
                if (lim < rnd)
                    goto retry;
                if (rnd < lim)
                    boundary = 0;
            }
        }
        else {
            rnd = 0;
        }
        BIG_SET32(val, i, (BDIGIT)rnd);
    }
    return rb_big_norm((VALUE)val);
}

unsigned long
rb_rand_internal(unsigned long i)
{
    struct MT *mt = &default_rand.rnd.mt;
    if (!genrand_initialized(mt)) {
	rand_init(mt, random_seed());
    }
    return limited_rand(mt, i);
}

/*
 * call-seq: prng.bytes(size) -> prng
 *
 * Returns a random binary string.  The argument size specified the length of
 * the result string.
 */
static VALUE
random_bytes(VALUE obj, VALUE len)
{
    rb_random_t *rnd = get_rnd(obj);
    long n = FIX2LONG(rb_to_int(len));
    VALUE bytes = rb_str_new(0, n);
    char *ptr = RSTRING_PTR(bytes);
    unsigned int r, i;

    for (; n >= SIZEOF_INT32; n -= SIZEOF_INT32) {
	r = genrand_int32(&rnd->mt);
	i = SIZEOF_INT32;
	do {
	    *ptr++ = (char)r;
	    r >>= CHAR_BIT;
        } while (--i);
    }
    if (n > 0) {
	r = genrand_int32(&rnd->mt);
	do {
	    *ptr++ = (char)r;
	    r >>= CHAR_BIT;
	} while (--n);
    }
    return bytes;
}

static VALUE
range_values(VALUE vmax, VALUE *begp)
{
    VALUE end, r, one = INT2FIX(1);
    int excl;

    if (!rb_range_values(vmax, begp, &end, &excl)) return Qfalse;
    if (!rb_respond_to(end, id_minus)) return Qfalse;
    r = rb_funcall2(end, id_minus, 1, begp);
    if (NIL_P(r)) return Qfalse;
    if (!excl && rb_respond_to(r, id_plus)) {
	r = rb_funcall2(r, id_plus, 1, &one);
	if (NIL_P(r)) return Qfalse;
    }
    return r;
}

static inline VALUE
add_to_begin(VALUE beg, VALUE offset)
{
    if (beg == Qundef) return offset;
    return rb_funcall2(beg, id_plus, 1, &offset);
}

static VALUE
rand_int(struct MT *mt, VALUE vmax)
{
    if (FIXNUM_P(vmax)) {
	long max = FIX2LONG(vmax);
	unsigned long r;
	if (!max) return Qnil;
	r = limited_rand(mt, (unsigned long)(max < 0 ? -max : max) - 1);
	return ULONG2NUM(r);
    }
    else {
	struct RBignum *limit = (struct RBignum *)vmax;
	if (rb_bigzero_p(vmax)) return Qnil;
	if (!RBIGNUM_SIGN(limit)) {
	    limit = (struct RBignum *)rb_big_clone(vmax);
	    RBIGNUM_SET_SIGN(limit, 1);
	}
	limit = (struct RBignum *)rb_big_minus((VALUE)limit, INT2FIX(1));
	if (FIXNUM_P((VALUE)limit)) {
	    if (FIX2LONG((VALUE)limit) == -1)
		return Qnil;
	    return LONG2NUM(limited_rand(mt, FIX2LONG((VALUE)limit)));
	}
	return limited_big_rand(mt, limit);
    }
}

/*
 * call-seq: prng.int(limit) -> integer
 *
 * When the argument is an +Integer+ or a +Bignum+, it returns a
 * random integer greater than or equal to zero and less than the
 * argument.  Unlike Random#rand, when the argument is a negative
 * integer or zero, it raises an ArgumentError.
 *
 * When the argument _limit_ is a +Range+, it returns a random
 * integer from integers where range.member?(integer) == true.
 *     prng.int(5..9)  # => one of [5, 6, 7, 8, 9]
 *     prng.int(5...9) # => one of [5, 6, 7, 8]
 *
 * +begin+/+end+ of the range have to have subtruct and add methods.
 *
 * Otherwise, it raises an ArgumentError.
 */
static VALUE
random_int(VALUE obj, VALUE vmax)
{
    VALUE v, beg = Qundef;
    rb_random_t *rnd = get_rnd(obj);

    v = rb_check_to_integer(vmax, "to_int");
    if (NIL_P(v)) {
	/* range like object support */
	if (!(v = range_values(vmax, &beg))) {
	    beg = Qundef;
	    NUM2LONG(vmax);
	}
    }
    v = rand_int(&rnd->mt, v);
    if (NIL_P(v)) v = INT2FIX(0);
    return add_to_begin(beg, v);
}

/*
 * call-seq:
 *     prng.float -> float
 *     prng.float([max=1.0]) -> float
 *
 * Returns a random floating point number between 0.0 and _max_,
 * including 0.0 and excluding _max_.
 */
static VALUE
random_float(int argc, VALUE *argv, VALUE obj)
{
    rb_random_t *rnd = get_rnd(obj);
    VALUE vmax, beg = Qundef;
    double max = 0, r;

    switch (argc) {
      case 0:
	break;
      case 1:
	vmax = argv[0];
	if (TYPE(vmax) == T_FLOAT ||
	    !NIL_P(vmax = rb_to_float(vmax)) ||
	    (vmax = range_values(vmax, &beg)) != Qfalse) {
	    max = RFLOAT_VALUE(vmax);
	}
	else {
	    beg = Qundef;
	    Check_Type(argv[0], T_FLOAT);
	}
	break;
      default:
	rb_scan_args(argc, argv, "01", 0);
	break;
    }
    r = genrand_real(&rnd->mt);
    if (argc) r *= max;
    return add_to_begin(beg, rb_float_new(r));
}

/*
 * call-seq:
 *     prng1 == prng2 -> true or false
 *
 * Returns true if the generators' states equal.
 */
static VALUE
random_equal(VALUE self, VALUE other)
{
    rb_random_t *r1, *r2;
    if (rb_obj_class(self) != rb_obj_class(other)) return Qfalse;
    r1 = get_rnd(self);
    r2 = get_rnd(other);
    if (!RTEST(rb_funcall2(r1->seed, rb_intern("=="), 1, &r2->seed))) return Qfalse;
    if (memcmp(r1->mt.state, r2->mt.state, sizeof(r1->mt.state))) return Qfalse;
    if ((r1->mt.next - r1->mt.state) != (r2->mt.next - r2->mt.state)) return Qfalse;
    if (r1->mt.left != r2->mt.left) return Qfalse;
    return Qtrue;
}

/*
 *  call-seq:
 *     rand(max=0)    => number
 *
 *  Converts <i>max</i> to an integer using max1 =
 *  max<code>.to_i.abs</code>. If the result is zero, returns a
 *  pseudorandom floating point number greater than or equal to 0.0 and
 *  less than 1.0. Otherwise, returns a pseudorandom integer greater
 *  than or equal to zero and less than max1. <code>Kernel::srand</code>
 *  may be used to ensure repeatable sequences of random numbers between
 *  different runs of the program. Ruby currently uses a modified
 *  Mersenne Twister with a period of 2**19937-1.
 *
 *     srand 1234                 #=> 0
 *     [ rand,  rand ]            #=> [0.191519450163469, 0.49766366626136]
 *     [ rand(10), rand(1000) ]   #=> [6, 817]
 *     srand 1234                 #=> 1234
 *     [ rand,  rand ]            #=> [0.191519450163469, 0.49766366626136]
 */

static VALUE
rb_f_rand(int argc, VALUE *argv, VALUE obj)
{
    VALUE vmax, r;
    struct MT *mt = &default_rand.rnd.mt;

    if (!genrand_initialized(mt)) {
	rand_init(mt, random_seed());
    }
    if (argc == 0) goto zero_arg;
    rb_scan_args(argc, argv, "01", &vmax);
    if (NIL_P(vmax)) goto zero_arg;
    vmax = rb_to_int(vmax);
    if (vmax == INT2FIX(0) || NIL_P(r = rand_int(mt, vmax))) {
      zero_arg:
	return DBL2NUM(genrand_real(mt));
    }
    return r;
}

void
Init_RandomSeed(void)
{
    fill_random_seed(default_rand.initial);
    init_by_array(&default_rand.rnd.mt, default_rand.initial, DEFAULT_SEED_CNT);
}

static void
Init_RandomSeed2(void)
{
    default_rand.rnd.seed = make_seed_value(default_rand.initial);
    memset(default_rand.initial, 0, DEFAULT_SEED_LEN);
}

void
rb_reset_random_seed(void)
{
    uninit_genrand(&default_rand.rnd.mt);
    default_rand.rnd.seed = INT2FIX(0);
}

void
Init_Random(void)
{
    Init_RandomSeed2();
    rb_define_global_function("srand", rb_f_srand, -1);
    rb_define_global_function("rand", rb_f_rand, -1);
    rb_global_variable(&default_rand.rnd.seed);

    rb_cRandom = rb_define_class("Random", rb_cObject);
    rb_define_alloc_func(rb_cRandom, random_alloc);
    rb_define_method(rb_cRandom, "initialize", random_init, -1);
    rb_define_method(rb_cRandom, "int", random_int, 1);
    rb_define_method(rb_cRandom, "bytes", random_bytes, 1);
    rb_define_method(rb_cRandom, "float", random_float, -1);
    rb_define_method(rb_cRandom, "seed", random_get_seed, 0);
    rb_define_method(rb_cRandom, "initialize_copy", random_copy, 1);
    rb_define_method(rb_cRandom, "marshal_dump", random_dump, 0);
    rb_define_method(rb_cRandom, "marshal_load", random_load, 1);
    rb_define_method(rb_cRandom, "state", random_state, 0);
    rb_define_method(rb_cRandom, "left", random_left, 0);
    rb_define_method(rb_cRandom, "==", random_equal, 1);

    rb_define_singleton_method(rb_cRandom, "srand", rb_f_srand, -1);
    rb_define_singleton_method(rb_cRandom, "rand", rb_f_rand, -1);
    rb_define_singleton_method(rb_cRandom, "new_seed", random_seed, 0);
    rb_define_singleton_method(rb_cRandom, "state", random_s_state, 0);
    rb_define_singleton_method(rb_cRandom, "left", random_s_left, 0);
}
