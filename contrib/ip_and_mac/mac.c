/*
 *	PostgreSQL type definitions for MAC addresses.
 */

#include <stdio.h>

#include <postgres.h>
#include <utils/palloc.h>

#include "mac.h"

/*
 *	This is the internal storage format for MAC addresses:
 */

typedef struct macaddr {
  unsigned char a;
  unsigned char b;
  unsigned char c;
  unsigned char d;
  unsigned char e;
  unsigned char f;
  short pad;
} macaddr;

/*
 *	Various forward declarations:
 */

macaddr *macaddr_in(char *str);
char *macaddr_out(macaddr *addr);

bool macaddr_eq(macaddr *a1, macaddr *a2);
bool macaddr_ne(macaddr *a1, macaddr *a2);

int4 macaddr_cmp(macaddr *a1, macaddr *a2);
bool macaddr_like(macaddr *a1, macaddr *a2);
text *macaddr_manuf(macaddr *addr);

/*
 *	Utility macros used for sorting and comparing:
 */

#define MagM(addr) \
  ((unsigned long)((addr->a<<16)|(addr->b<<8)|(addr->c)))

#define MagH(addr) \
  ((unsigned long)((addr->c<<16)|(addr->e<<8)|(addr->f)))

/*
 *	MAC address reader.  Accepts several common notations.
 */

macaddr *macaddr_in(char *str) {
  int a, b, c, d, e, f;
  macaddr *result;
  int count;

  if (strlen(str) > 0) {

    count = sscanf(str, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
    if (count != 6)
      count = sscanf(str, "%x-%x-%x-%x-%x-%x", &a, &b, &c, &d, &e, &f);
    if (count != 6)
      count = sscanf(str, "%2x%2x%2x:%2x%2x%2x", &a, &b, &c, &d, &e, &f);
    if (count != 6)
      count = sscanf(str, "%2x%2x%2x-%2x%2x%2x", &a, &b, &c, &d, &e, &f);
    if (count != 6)
      count = sscanf(str, "%2x%2x.%2x%2x.%2x%2x", &a, &b, &c, &d, &e, &f);
    
    if (count != 6) {
      elog(ERROR, "macaddr_in: error in parsing \"%s\"", str);
      return(NULL);
    }
    
    if ((a < 0) || (a > 255) || (b < 0) || (b > 255) ||
	(c < 0) || (c > 255) || (d < 0) || (d > 255) ||
	(e < 0) || (e > 255) || (f < 0) || (f > 255)) {
      elog(ERROR, "macaddr_in: illegal address \"%s\"", str);
      return(NULL);
    }
  } else {
    a = b = c = d = e = f = 0;	/* special case for missing address */
  }

  result = (macaddr *)palloc(sizeof(macaddr));

  result->a = a;
  result->b = b;
  result->c = c;
  result->d = d;
  result->e = e;
  result->f = f;

  return(result);
}

/*
 *	MAC address output function.  Fixed format.
 */

char *macaddr_out(macaddr *addr) {
  char *result;

  if (addr == NULL)
    return(NULL);

  result = (char *)palloc(32);

  if ((MagM(addr) > 0) || (MagH(addr) > 0)) {
    sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x",
	    addr->a, addr->b, addr->c, addr->d, addr->e, addr->f);
  } else {
    result[0] = 0;		/* special case for missing address */
  }
  return(result);
}

/*
 *	Boolean tests.
 */

bool macaddr_eq(macaddr *a1, macaddr *a2) {
  return((a1->a == a2->a) && (a1->b == a2->b) &&
	 (a1->c == a2->c) && (a1->d == a2->d) &&
	 (a1->e == a2->e) && (a1->f == a2->f));
};

bool macaddr_ne(macaddr *a1, macaddr *a2) {
  return((a1->a != a2->a) || (a1->b != a2->b) ||
	 (a1->c != a2->c) || (a1->d != a2->d) ||
	 (a1->e != a2->e) || (a1->f != a2->f));
};

/*
 *	Comparison function for sorting:
 */

int4 macaddr_cmp(macaddr *a1, macaddr *a2) {
  unsigned long a1magm, a1magh, a2magm, a2magh;
  a1magm = MagM(a1);
  a1magh = MagH(a1);
  a2magm = MagM(a2);
  a2magh = MagH(a2);
  if (a1magm < a2magm)
    return -1;
  else if (a1magm > a2magm)
    return 1;
  else if (a1magh < a2magh)
    return -1;
  else if (a1magh > a2magh)
    return 1;
  else
    return 0;
}

/*
 *	Similarity means having the same manufacurer, which means
 *	having the same first three bytes of address:
 */

bool macaddr_like(macaddr *a1, macaddr *a2) {
  unsigned long a1magm, a2magm;
  a1magm = MagM(a1);
  a2magm = MagM(a2);
  if ((a1magm == 0) || (a2magm == 0))
    return FALSE;
  return (a1magm == a2magm);
}

/*
 *	The special manufacturer fetching function.  See "mac.h".
 */

text *macaddr_manuf(macaddr *addr) {
  manufacturer *manuf;
  int length;
  text *result;

  for (manuf = manufacturers; manuf->name != NULL; manuf++) {
    if ((manuf->a == addr->a) &&
	(manuf->b == addr->b) &&
	(manuf->c == addr->c))
      break;
  }
  if (manuf->name == NULL) {
    result = palloc(VARHDRSZ + 1);
    memset(result, 0, VARHDRSZ + 1);
    VARSIZE(result) = VARHDRSZ + 1;
  } else {
    length = strlen(manuf->name) + 1;
    result = palloc(length + VARHDRSZ);
    memset(result, 0, length + VARHDRSZ);
    VARSIZE(result) = length + VARHDRSZ;
    memcpy(VARDATA(result), manuf->name, length);
  }
  return result;
}

/*
 *	eof
 */
