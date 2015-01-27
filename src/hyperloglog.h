#ifndef _HYPERLOGLOG_H_
#define _HYPERLOGLOG_H_
/* This is an implementation of HyperLogLog algorithm as described in the
 * paper "HyperLogLog: the analysis of near-optimal cardinality estimation
 * algorithm", published by Flajolet, Fusy, Gandouet and Meunier in 2007.
 * Generally it is an improved version of LogLog algorithm with the last
 * step modified, to combine the parts using harmonic means.
 *
 * Several improvements have been included that are described in "HyperLogLog 
 * in Practice: Algorithmic Engineering of a State of The Art Cardinality 
 * Estimation Algorithm", published by Stefan Heulem, Marc Nunkesse and 
 * Alexander Hall.
 *
 * ----------------------------------------------------------------------------
 * DEFINED CONSTANTS 
 *
 * ERROR_CONST = 1.04*1.04
 * 
 * 	Used in calculating the minimum m for a desired error_rate. Derived from 
 * 	error_rate = 1.04/sqrt(m)
 * 	sqrt(m) = 1.04/error_rate
 * 	m = (1.04/error_rate)^2
 * 	m = 1.0816/(error_rate*error_rate) 
 *
 * MIN_INDEX_BITS no real sense in being as innacurate as <4 values would be (>35%)
 *
 * MAX_INDEX_BITS error correction data only goes up to 18
 *
 * HASH_LENGTH the version of MurmurHash we use produces 64 bit hashes 
 *
 * HASH_SEED a random seed for the hash function to use
 *
 * MAX_INTERPOLATION_POINTS any precision (# index bits) above 5 has 200 points 
 *
 * PRECISION_5_MAX_INTERPOLATION_POINTS precision 5 only has 159 points
 *
 * PRECISION_4_MAX_INTERPOLATION_POINTS precision 4 only has 79 points 
 * 
 * STRUCT_VERSION
 * 0 - Basic implementation + compression and H++'s improved error correction at
 * low cardinalities
 *
 * 1 - Sparse encoding added for low cardinialities. Improves accuracy and storage
 * for low cardinalities. */
#define ERROR_CONST  1.0816
#define MIN_INDEX_BITS 4
#define MAX_INDEX_BITS 18
#define HASH_LENGTH 64
#define HASH_SEED 0xadc83b19ULL
#define MAX_INTERPOLATION_POINTS 200
#define PRECISION_5_MAX_INTERPOLATION_POINTS 159
#define PRECISION_4_MAX_INTERPOLATION_POINTS 79
#define STRUCT_VERSION 2

#define HLL_DENSE_GET_REGISTER(target,p,regnum,hll_bits) do { \
    uint8_t *_p = (uint8_t*) p; \
    unsigned long _byte = regnum*hll_bits/8; \
    unsigned long _fb = regnum*hll_bits&7; \
    unsigned long _fb8 = 8 - _fb; \
    unsigned long b0 = _p[_byte]; \
    unsigned long b1 = _p[_byte+1]; \
    target = ((b0 >> _fb) | (b1 << _fb8)) & ((1<<hll_bits)-1); \
} while(0)

/* Set the value of the register at position 'regnum' to 'val'.
 * 'p' is an array of unsigned bytes. */
#define HLL_DENSE_SET_REGISTER(p,regnum,val,hll_bits) do { \
    uint8_t *_p = (uint8_t*) p; \
    unsigned long _byte = regnum*hll_bits/8; \
    unsigned long _fb = regnum*hll_bits&7; \
    unsigned long _fb8 = 8 - _fb; \
    unsigned long _v = val; \
    _p[_byte] &= ~(((1<<hll_bits)-1) << _fb); \
    _p[_byte] |= _v << _fb; \
    _p[_byte+1] &= ~(((1<<hll_bits)-1) >> _fb8); \
    _p[_byte+1] |= _v >> _fb8; \
} while(0)

/* type defininitions */
typedef struct HyperLogLogCounterData {
    
    /* length of the structure (varlena) used heavily by postgres internally */
    char vl_len_[4];
    
    /* Number bits used to index the buckets - this is determined depending
     * on the requested error rate - see hyperloglog_create() for details. 
     * This variable is unsigned as a negative version is used to indicate
     * the data is compressed and requires decompression */
    int8_t b; /* bits for bin index */
    
    /* number of bits for a single bucket */
    uint8_t binbits;

    /* Used to indicate the version of the struct to allow further modification in hte future */
    uint16_t version;
   
    /* The current index of the sparse encoded data array. Also when -1 used as a flag for dense
     * encoded counters */
    int32_t idx;
 
    /* largest observed 'rho' for each of the 'm' buckets (uses the very same trick
     * as in the varlena type in include/c.h where additional memory is palloc'ed and
     * treated as part of the data array ) */
    char data[1];
    
} HyperLogLogCounterData;

typedef HyperLogLogCounterData * HyperLogLogCounter;


/* creates an optimal bitmap able to count a multiset with the expected
 * cardinality and the given error rate. */
HyperLogLogCounter hyperloglog_create(double ndistinct, float error);

/* Helper function to return the size of a fully populated counter with
 * the given parameters. */
int hyperloglog_get_size(double ndistinct, float error);

/* Compares the bucket values of two counters to test for equality */
int hyperloglog_is_equal(HyperLogLogCounter counter1, HyperLogLogCounter counter2);

/* Returns a copy of the counter */
HyperLogLogCounter hyperloglog_copy(HyperLogLogCounter counter);

/* Merges two counters into one. The final counter can either be a modified counter1 or
 * completely new copy. */
HyperLogLogCounter hyperloglog_merge(HyperLogLogCounter counter1, HyperLogLogCounter counter2, short inplace);

/* add element existence */
HyperLogLogCounter hyperloglog_add_element(HyperLogLogCounter hloglog, const char * element, int elen);

/* get an estimate from the hyperloglog counter */
double hyperloglog_estimate(HyperLogLogCounter hloglog);

/* reset a counter */
void hyperloglog_reset_internal(HyperLogLogCounter hloglog);

/* data compression/decompression */
HyperLogLogCounter hyperloglog_compress(HyperLogLogCounter hloglog);
HyperLogLogCounter hyperloglog_decompress(HyperLogLogCounter hloglog);

#endif // #ifndef _HYPERLOGLOG_H_
