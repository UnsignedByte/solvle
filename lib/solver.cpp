/*
* @Author: UnsignedByte
* @Date:   2022-02-17 09:08:40
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-18 23:39:02
*/

#include "solver.hpp"
#include <cstdio>
#include <iostream>
#include <cstring>

#include <pthread.h>

namespace solver {

	namespace {
		size_t pos = 0;
		char* words = nullptr;

		// number of valid
		size_t lvalid = 0;
		size_t lsolutions = 0;

		// data
		char* valid = nullptr;
		char* solutions = nullptr;

		// Read a file of 5 character strings line by line, returns the number of files.
		size_t readfile (char*& str, const char* name) {
			FILE* f = fopen(name, "r");

			fseek(f, 0L, SEEK_END);
			long s = ftell(f);
			fseek(f, 0L, SEEK_SET);

			s = (s + 1) / 6 * 5; // Remove newlines from length

			str = static_cast<char*>(malloc(s * sizeof(char)));

			for(char* c = str; c - str < s; c += 5) {
				fgets(c, 6, f); // fgets reads up to size - 1
				fgetc(f); // discard
			}

			for (char* c = str; *c; ++c) {
				*c = toupper((unsigned char) *c);
			}

			return s / 5;
		}

		
		struct wstate {
			char* valid;
			char* solutions;
			int lvalid;
			int lsolutions;

			wstate () {
				lvalid = 0;
				lsolutions = 0;
			}

			~wstate () {
				free(valid);
				free(solutions);
			}
		};

		uint8_t* gstate( const char* guess, const char* answer ) {
			wstate ws;
			// 4 Parts:
			// 26 8 bit ints, low 5 bits of each represents whether the nth character must be in that position of the string
			// 26 8 bit ints, low 5 bits of each represents whether the nth character cant be in that position of the string
			// 26 8 bit ints, representing minimum count of each letter in ans
			// 26 8 bit ints, representing maximum count of each letter in ans
			uint8_t* state = static_cast<uint8_t*>(malloc(26 * 3 * sizeof(uint8_t)));
			const int rmust = 0;
			const int rcant = 26;
			const int rmin = 26 * 2;
			const int rmax = 26 * 3;
			for (int i = 0; i < 26; ++i) *(state + i + rmust) = 0;
			for (int i = 0; i < 26; ++i) *(state + i + rcant) = 0;
			for (int i = 0; i < 26; ++i) *(state + i + rmin) = 0;
			for (int i = 0; i < 26; ++i) *(state + i + rmax) = 5; // 5 is default max

			// count number of occurrences of each letter in the string
			uint8_t* counts = static_cast<uint8_t*>(malloc(26 * sizeof(uint8_t)));

			uint8_t mask = 31;

			for (int i = 0; i < 26; ++i) *(counts + i) = 0;

			for (int j = 0; j < 5; ++j) {
				*(counts + *(answer + j) - 'A')++; // increment count
			}

			// Pass 1: detect green letters
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (c == *(answer + i)) {
					mask ^= 1 << i;
					*(counts + c - 'A')--; // decrement count
					*(ws.state + c - 'A' + rmust) |= 1 << i;
					*(ws.state + c - 'A' + rmin)++; // increment min count
				}
			}

			// Pass 2: yellow letters
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (mask & 1 << i && *(counts + c - 'A') > 0) { // if all the counts in the answer haven't already been matched
					mask ^= 1 << i;
					*(counts + c - 'A')--;
					*(ws.state + c - 'A' + rcant) |= 1 << i;
					*(ws.state + c - 'A' + rmin)++; // increment min count
				}
			}

			// Pass 2: remaining gray
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (mask & 1 << i) { // all unmatched letters
					*(ws.state + c - 'A' + rmax) = *(ws.state + c - 'A' + rmin); // Set max to min
				}
			}

			free(counts);

			return state;
		}


/*
DATA FORMAT

<int L>
<string s>
<int L2>
<string s2>
...
<string s>
<int L2>
...
<string s>
*/
		struct dfsta {
			int id; // output filename
			char* gset; // guess set start
			int l; // number of words allocated to this thread
		};
		void* dfst ( void* arg ) {
			dfsta args = *((dfsta*) arg);

			printf("Starting thread %d. %d words allocated.\n", args.id, args.l);

			char* fout;
			sprintf(fout, "data/tmp/tmp-%d.dat", i);

			FILE* out = fopen(fout, "w");



			return nullptr;
		}
	}

	void init ( const char* fout, const char* fvalid, const char* fsolutions, const int threadcount ) {
		// Read in files to the char*
		lvalid = readfile(valid, fvalid);
		lsolutions = readfile(solutions, fsolutions);

		// reset current guesses
		pos = 0;
		words = static_cast<char*>(malloc(30*sizeof(char)));
		strcpy(words, "                              "); // fill with spaces

		pthread_t* threads = static_cast<pthread_t*>(malloc(threadcount * sizeof(pthread_t)));
		int* trets = static_cast<int*>(malloc(threadcount * sizeof(int)));

		int ll = lvalid;

		for (int i = 0; i < threadcount; ++i) {
			int alloc = ll / (threadcount - i);
			ll -= alloc;

			dfsta args = {i, valid + (lvalid - ll) * 5, ll};

			*(trets + i) = pthread_create(threads + i, NULL, dfst, (void*) &args);
		}

		for (int i = 0; i < threadcount; ++i) {
			pthread_join(*(threads + i), NULL);

			printf("Thread %d returns with %d\n", i, *(trets + i));
		}
	}


	char* getstate () {
		char* p = static_cast<char*>(malloc(30*sizeof(char)));
		return strcpy(p, words);
	}

	void charEvent ( int code ) {
		bool s = *(words + pos) == ' '; // if the cursor points to an empty character
		bool e = ((pos+1) % 5) > 0; // if the cursor is at the end of the line

		if (0 <= code && code < 26 && s) {
			*(words + pos) = 'A' + code;

			pos += e;
		} else if (code == 59 && pos % 5 > 0) {
			pos -= s || e;
			*(words + pos) = ' ';
		}

	}

}