/**
 * \file bignum.h
 *
 *  Based on TropicSSL: Copyright (C) 2017 Shanghai Real-Thread Technology Co., Ltd
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TINY_CRYPT_BIGNUM_H__
#define TINY_CRYPT_BIGNUM_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef signed long tiny_mpi_sint;
typedef unsigned long tiny_mpi_uint;

/**
 * \brief          MPI structure
 */
typedef struct
{
    int s;                          /*!<  integer sign      */
    int n;                          /*!<  total # of limbs  */
    tiny_mpi_uint *p;            /*!<  pointer to limbs  */
}
tiny_mpi;

/**
 * \brief           Initialize one MPI (make internal references valid)
 *                  This just makes it ready to be set or freed,
 *                  but does not define a value for the MPI.
 *
 * \param X         One MPI to initialize.
 */
void tiny_mpi_init(tiny_mpi *X);

/**
 * \brief          Unallocate one MPI
 *
 * \param X        One MPI to unallocate.
 */
void tiny_mpi_free(tiny_mpi *X);

/**
 * \brief          Enlarge to the specified number of limbs
 *
 * \param X        MPI to grow
 * \param nblimbs  The target number of limbs
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_grow(tiny_mpi *X, int nblimbs);

/**
 * \brief          Resize down, keeping at least the specified number of limbs
 *
 * \param X        MPI to shrink
 * \param nblimbs  The minimum number of limbs to keep
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_shrink(tiny_mpi *X, int nblimbs);

/**
 * \brief          Copy the contents of Y into X
 *
 * \param X        Destination MPI
 * \param Y        Source MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_copy(tiny_mpi *X, const tiny_mpi *Y);

/**
 * \brief          Swap the contents of X and Y
 *
 * \param X        First MPI value
 * \param Y        Second MPI value
 */
void tiny_mpi_swap(tiny_mpi *X, tiny_mpi *Y);

/**
 * \brief          Safe conditional assignement X = Y if assign is 1
 *
 * \param X        MPI to conditionally assign to
 * \param Y        Value to be assigned
 * \param assign   1: perform the assignment, 0: keep X's original value
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *
 * \note           This function is equivalent to
 *                      if( assign ) tiny_mpi_copy( X, Y );
 *                 except that it avoids leaking any information about whether
 *                 the assignment was done or not (the above code may leak
 *                 information through branch prediction and/or memory access
 *                 patterns analysis).
 */
int tiny_mpi_safe_cond_assign(tiny_mpi *X, const tiny_mpi *Y, unsigned char assign);

/**
 * \brief          Safe conditional swap X <-> Y if swap is 1
 *
 * \param X        First tiny_mpi value
 * \param Y        Second tiny_mpi value
 * \param assign   1: perform the swap, 0: keep X and Y's original values
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *
 * \note           This function is equivalent to
 *                      if( assign ) tiny_mpi_swap( X, Y );
 *                 except that it avoids leaking any information about whether
 *                 the assignment was done or not (the above code may leak
 *                 information through branch prediction and/or memory access
 *                 patterns analysis).
 */
int tiny_mpi_safe_cond_swap(tiny_mpi *X, tiny_mpi *Y, unsigned char assign);

/**
 * \brief          Set value from integer
 *
 * \param X        MPI to set
 * \param z        Value to use
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_lset(tiny_mpi *X, tiny_mpi_sint z);

/**
 * \brief          Get a specific bit from X
 *
 * \param X        MPI to use
 * \param pos      Zero-based index of the bit in X
 *
 * \return         Either a 0 or a 1
 */
int tiny_mpi_get_bit(const tiny_mpi *X, int pos);

/**
 * \brief          Set a bit of X to a specific value of 0 or 1
 *
 * \note           Will grow X if necessary to set a bit to 1 in a not yet
 *                 existing limb. Will not grow if bit should be set to 0
 *
 * \param X        MPI to use
 * \param pos      Zero-based index of the bit in X
 * \param val      The value to set the bit to (0 or 1)
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_BAD_INPUT_DATA if val is not 0 or 1
 */
int tiny_mpi_set_bit(tiny_mpi *X, int pos, unsigned char val);

/**
 * \brief          Return the number of zero-bits before the least significant
 *                 '1' bit
 *
 * Note: Thus also the zero-based index of the least significant '1' bit
 *
 * \param X        MPI to use
 */
int tiny_mpi_lsb(const tiny_mpi *X);

/**
 * \brief          Return the number of bits up to and including the most
 *                 significant '1' bit'
 *
 * Note: Thus also the one-based index of the most significant '1' bit
 *
 * \param X        MPI to use
 */
int tiny_mpi_bitlen(const tiny_mpi *X);

/**
 * \brief          Return the total size in bytes
 *
 * \param X        MPI to use
 */
int tiny_mpi_size(const tiny_mpi *X);

/**
 * \brief          Import X from unsigned binary data, big endian
 *
 * \param X        Destination MPI
 * \param buf      Input buffer
 * \param buflen   Input buffer size
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_read_binary(tiny_mpi *X, const unsigned char *buf, int buflen);

/**
 * \brief          Export X into unsigned binary data, big endian.
 *                 Always fills the whole buffer, which will start with zeros
 *                 if the number is smaller.
 *
 * \param X        Source MPI
 * \param buf      Output buffer
 * \param buflen   Output buffer size
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_BUFFER_TOO_SMALL if buf isn't large enough
 */
int tiny_mpi_write_binary(const tiny_mpi *X, unsigned char *buf, int buflen);

/**
 * \brief          Left-shift: X <<= count
 *
 * \param X        MPI to shift
 * \param count    Amount to shift
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_shift_l(tiny_mpi *X, int count);

/**
 * \brief          Right-shift: X >>= count
 *
 * \param X        MPI to shift
 * \param count    Amount to shift
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_shift_r(tiny_mpi *X, int count);

/**
 * \brief          Compare unsigned values
 *
 * \param X        Left-hand MPI
 * \param Y        Right-hand MPI
 *
 * \return         1 if |X| is greater than |Y|,
 *                -1 if |X| is lesser  than |Y| or
 *                 0 if |X| is equal to |Y|
 */
int tiny_mpi_cmp_abs(const tiny_mpi *X, const tiny_mpi *Y);

/**
 * \brief          Compare signed values
 *
 * \param X        Left-hand MPI
 * \param Y        Right-hand MPI
 *
 * \return         1 if X is greater than Y,
 *                -1 if X is lesser  than Y or
 *                 0 if X is equal to Y
 */
int tiny_mpi_cmp_mpi(const tiny_mpi *X, const tiny_mpi *Y);

/**
 * \brief          Compare signed values
 *
 * \param X        Left-hand MPI
 * \param z        The integer value to compare to
 *
 * \return         1 if X is greater than z,
 *                -1 if X is lesser  than z or
 *                 0 if X is equal to z
 */
int tiny_mpi_cmp_int(const tiny_mpi *X, tiny_mpi_sint z);

/**
 * \brief          Unsigned addition: X = |A| + |B|
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_add_abs(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Unsigned subtraction: X = |A| - |B|
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_NEGATIVE_VALUE if B is greater than A
 */
int tiny_mpi_sub_abs(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Signed addition: X = A + B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_add_mpi(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Signed subtraction: X = A - B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_sub_mpi(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Signed addition: X = A + b
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The integer value to add
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_add_int(tiny_mpi *X, const tiny_mpi *A, tiny_mpi_sint b);

/**
 * \brief          Signed subtraction: X = A - b
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The integer value to subtract
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_sub_int(tiny_mpi *X, const tiny_mpi *A, tiny_mpi_sint b);

/**
 * \brief          Baseline multiplication: X = A * B
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_mul_mpi(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Baseline multiplication: X = A * b
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param b        The unsigned integer value to multiply with
 *
 * \note           b is unsigned
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_mul_int(tiny_mpi *X, const tiny_mpi *A, tiny_mpi_uint b);

/**
 * \brief          Division by tiny_mpi: A = Q * B + R
 *
 * \param Q        Destination MPI for the quotient
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_DIVISION_BY_ZERO if B == 0
 *
 * \note           Either Q or R can be NULL.
 */
int tiny_mpi_div_mpi(tiny_mpi *Q, tiny_mpi *R, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Division by int: A = Q * b + R
 *
 * \param Q        Destination MPI for the quotient
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param b        Integer to divide by
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_DIVISION_BY_ZERO if b == 0
 *
 * \note           Either Q or R can be NULL.
 */
int tiny_mpi_div_int(tiny_mpi *Q, tiny_mpi *R, const tiny_mpi *A, tiny_mpi_sint b);

/**
 * \brief          Modulo: R = A mod B
 *
 * \param R        Destination MPI for the rest value
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_DIVISION_BY_ZERO if B == 0,
 *                 TINY_ERR_MPI_NEGATIVE_VALUE if B < 0
 */
int tiny_mpi_mod_mpi(tiny_mpi *R, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Modulo: r = A mod b
 *
 * \param r        Destination tiny_mpi_uint
 * \param A        Left-hand MPI
 * \param b        Integer to divide by
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_DIVISION_BY_ZERO if b == 0,
 *                 TINY_ERR_MPI_NEGATIVE_VALUE if b < 0
 */
int tiny_mpi_mod_int(tiny_mpi_uint *r, const tiny_mpi *A, tiny_mpi_sint b);

/**
 * \brief          Sliding-window exponentiation: X = A^E mod N
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param E        Exponent MPI
 * \param N        Modular MPI
 * \param _RR      Speed-up MPI used for recalculations
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_BAD_INPUT_DATA if N is negative or even or
 *                 if E is negative
 *
 * \note           _RR is used to avoid re-computing R*R mod N across
 *                 multiple calls, which speeds up things a bit. It can
 *                 be set to NULL if the extra performance is unneeded.
 */
int tiny_mpi_exp_mod(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *E, const tiny_mpi *N, tiny_mpi *_RR);

/**
 * \brief          Greatest common divisor: G = gcd(A, B)
 *
 * \param G        Destination MPI
 * \param A        Left-hand MPI
 * \param B        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed
 */
int tiny_mpi_gcd(tiny_mpi *G, const tiny_mpi *A, const tiny_mpi *B);

/**
 * \brief          Modular inverse: X = A^-1 mod N
 *
 * \param X        Destination MPI
 * \param A        Left-hand MPI
 * \param N        Right-hand MPI
 *
 * \return         0 if successful,
 *                 TINY_ERR_MPI_ALLOC_FAILED if memory allocation failed,
 *                 TINY_ERR_MPI_BAD_INPUT_DATA if N is <= 1,
                   TINY_ERR_MPI_NOT_ACCEPTABLE if A has no inverse mod N.
 */
int tiny_mpi_inv_mod(tiny_mpi *X, const tiny_mpi *A, const tiny_mpi *N);

#ifdef __cplusplus
}
#endif

#endif
