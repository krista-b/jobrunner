/*
 * csse2310a3.h
 */

#ifndef CSSE2310A3_H
#define CSSE2310A3_H

#include <stdio.h>

// read the next line of the text from the given FILE. 
// The string returned is allocated using malloc() and the newline (if any)
// is stripped from the end of the line. Returns NULL on EOF.
char* read_line(FILE*);
// The readLine function has now been deprecated in favour of read_line().
char* readLine(FILE*) __attribute__((deprecated("Please use read_line() instead")));


// Return an array of pointers to all of the comma-separated fields within
// the given line of text - very much like the argv array passed to main(). 
// (The commas in the given line of text are replaced with null (0) 
// characters.) The space needed for the array is allocated using malloc(). 
// Element 0 of the returned array is always equal to line. The last 
// element of the returned array will always equal NULL (like the argv 
// array passed to main().)
char** split_by_commas(char* line);
// The splitByCommas function has now been deprecated in favour of split_by_commas().
char** splitByCommas(char* line) __attribute__((deprecated("Please use split_by_commas() instead")));


#endif
