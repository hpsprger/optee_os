/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Form ABI specifications:
 *      int __aeabi_idiv(int numerator, int denominator);
 *     unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator);
 *
 *     typedef struct { int quot; int rem; } idiv_return;
 *     typedef struct { unsigned quot; unsigned rem; } uidiv_return;
 *
 *     __value_in_regs idiv_return __aeabi_idivmod(int numerator,
 *     int *denominator);
 *     __value_in_regs uidiv_return __aeabi_uidivmod(unsigned *numerator,
 *     unsigned denominator);
 */

/* struct qr - stores qutient/remainder to handle divmod EABI interfaces. */
struct qr {
	unsigned q;		/* computed quotient */
	unsigned r;		/* computed remainder */
	unsigned q_n;		/* specficies if quotient shall be negative */
	unsigned r_n;		/* specficies if remainder shall be negative */
};

static void uint_div_qr(unsigned numerator, unsigned denominator,
			struct qr *qr);

/* returns in R0 and R1 by tail calling an asm function */
unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator);

unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator);
unsigned __aeabi_uimod(unsigned numerator, unsigned denominator);

/* returns in R0 and R1 by tail calling an asm function */
signed __aeabi_idivmod(signed numerator, signed denominator);

signed __aeabi_idiv(signed numerator, signed denominator);
signed __aeabi_imod(signed numerator, signed denominator);

/*
 * __ste_idivmod_ret_t __aeabi_idivmod(signed numerator, signed denominator)
 * Numerator and Denominator are received in R0 and R1.
 * Where __ste_idivmod_ret_t is returned in R0 and R1.
 *
 * __ste_uidivmod_ret_t __aeabi_uidivmod(unsigned numerator,
 *                                       unsigned denominator)
 * Numerator and Denominator are received in R0 and R1.
 * Where __ste_uidivmod_ret_t is returned in R0 and R1.
 */
#ifdef __GNUC__
signed ret_idivmod_values(signed quotient, signed remainder);
unsigned ret_uidivmod_values(unsigned quotient, unsigned remainder);
#else
#error "Compiler not supported"
#endif

static void division_qr(int n, int p, struct qr *qr)
{
	int i = 0, q = 0;
	if (p == 0) {
		qr->r = 0xFFFFFFFF;	/* division by 0 */
		return;
	}

	if (n >= p)
		i = (i << 1) + 1;	/* increase size of q */

	while (n>=(p<<1)) {
		i = (i << 1) + 1;	/* increase size of q */
		p = p << 1;		/* increase p */
	}

	while (i>0) {
		q = q << 1;	/* write bit in q at index (size-1) */
		while (n >= p)
		{
			n -= p;
			q++;
		}
		p = p >> 1; 	/* decrease p */
		i = i >> 1; 	/* decrease remaining size in q */
	}
	qr->r = n;
	qr->q = q;
}

static void uint_div_qr(unsigned numerator, unsigned denominator, struct qr *qr)
{
	struct qr qr2;

	/*
	 * division_qr support dividing 31 bit unsigned numbers
	 * encoded on 32bit registers. In case value is to high,
	 * perform 2 divisions, one for each halves, and accumulate
	 * quotient and remainders.
	 */
	if (numerator & (1 << 31)) {
		unsigned num = numerator >> 1;
		division_qr(num, denominator, qr);
		division_qr(num + (numerator & 1), denominator, &qr2);
		qr->q += qr2.q;
		qr->r += qr2.r;
		if (qr->r >= denominator) {
			qr->q++;
			qr->r -= denominator;
		}
		if (qr->r >= denominator) {
			qr->q++;
			qr->r -= denominator;
		}
	} else {
		division_qr(numerator, denominator, qr);
	}
	/* negate quotient and/or remainder according to requester */
	if (qr->q_n)
		qr->q = -qr->q;
	if (qr->r_n)
		qr->r = -qr->r;
}

unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator)
{
	struct qr qr = { .q_n = 0, .r_n = 0 };

	uint_div_qr(numerator, denominator, &qr);

	return qr.q;
}

unsigned __aeabi_uimod(unsigned numerator, unsigned denominator)
{
	struct qr qr = { .q_n = 0, .r_n = 0 };

	uint_div_qr(numerator, denominator, &qr);

	return qr.r;
}

unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator)
{
	struct qr qr = { .q_n = 0, .r_n = 0 };

	uint_div_qr(numerator, denominator, &qr);

	return ret_uidivmod_values(qr.q, qr.r);
}

signed __aeabi_idiv(signed numerator, signed denominator)
{
	struct qr qr = { .q_n = 0, .r_n = 0 };

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);

	return qr.q;
}

signed __aeabi_imod(signed numerator, signed denominator)
{
	signed s;
	signed i;
	signed j;
	signed h;

	struct qr qr = { .q_n = 0, .r_n = 0 };

	/* in case modulo of a power of 2 */
	for (i = 0, j = 0, h = 0, s = denominator; (s != 0) || (h > 1); i++) {
		if (s & 1) {
			j = i;
			h++;
		}
		s = s >> 1;
	}
	if (h == 1)
		return numerator >> j;

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);

	return qr.r;
}

signed __aeabi_idivmod(signed numerator, signed denominator)
{
	struct qr qr = { .q_n = 0, .r_n = 0 };

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);

	return ret_idivmod_values(qr.q, qr.r);
}
