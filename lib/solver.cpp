/*
* @Author: UnsignedByte
* @Date:   2022-02-17 09:08:40
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-19 10:42:05
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

		// data
		struct wstate {
			char* valid = nullptr;
			char* solutions = nullptr;
			int lvalid = 0;
			int lsolutions = 0;

			void print ( bool v = false ) const {
				printf("Wstate with %d valid, %d solutions\n", lvalid, lsolutions);
				if (v) {
					printf("valid: %s\n\nsolutions: %s\n\n", valid, solutions);
				}
			}

			~wstate () {
				free(valid);
				free(solutions);
			}
		};

		wstate rootstate;

		// allocate a char* string
		char* salloc (int len, size_t size) {
			return static_cast<char*>(calloc(len + 1, size));
		}

		// Read a file of 5 character strings line by line, returns the number of files.
		size_t readfile (char*& str, const char* name) {
			FILE* f = fopen(name, "r");

			fseek(f, 0L, SEEK_END);
			long s = ftell(f);
			fseek(f, 0L, SEEK_SET);

			s = (s + 1) / 6 * 5; // Remove newlines from length

			str = salloc(s, sizeof(char));

			for(char* c = str; c - str < s; c += 5) {
				fgets(c, 6, f); // fgets reads up to size - 1
				fgetc(f); // discard
			}

			if (fclose(f) != 0) {
				printf("Failed to close file %s after reading.\n", name);
			}

			for (char* c = str; *c; ++c) {
				*c = toupper((unsigned char) *c);
			}

			return s / 5;
		}

		// count number of occurrences of each letter in the string
		uint8_t* countchars ( const char* word ) {
			printf("Test2%lu\n", sizeof(uint8_t));
			uint8_t* charcounts = static_cast<uint8_t*>(calloc(26, sizeof(uint8_t)));
			printf("Test3\n");

			for (int j = 0; j < 5; ++j) {
				(*(charcounts + *(word + j) - 'A'))++; // increment count
			}

			return charcounts;
		}

		const int rmust = 0;
		const int rcant = 26;
		const int rmin = 26 * 2;
		const int rmax = 26 * 3;

		uint8_t* gstate( const char* guess, const char* answer ) {
			wstate ws;
			// 4 Parts:
			// 26 8 bit ints, low 5 bits of each represents whether the nth character must be in that position of the string
			// 26 8 bit ints, low 5 bits of each represents whether the nth character cant be in that position of the string
			// 26 8 bit ints, representing minimum count of each letter in ans
			// 26 8 bit ints, representing maximum count of each letter in ans
			uint8_t* state = static_cast<uint8_t*>(calloc(26 * 4, sizeof(uint8_t)));
			// for (int i = 0; i < 26; ++i) *(state + i + rmust) = 0;
			// for (int i = 0; i < 26; ++i) *(state + i + rcant) = 0;
			// for (int i = 0; i < 26; ++i) *(state + i + rmin) = 0;
			for (int i = 0; i < 26; ++i) *(state + i + rmax) = 5; // 5 is default max

			uint8_t mask = 31;
			printf("Test\n");
			uint8_t* counts = countchars(answer);

			// Pass 1: detect green letters
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (c == *(answer + i)) {
					mask ^= 1 << i;
					(*(counts + c - 'A'))--; // decrement count
					*(state + c - 'A' + rmust) |= 1 << i;
					(*(state + c - 'A' + rmin))++; // increment min count
				}
			}

			// Pass 2: yellow letters
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (mask & 1 << i && *(counts + c - 'A') > 0) { // if all the counts in the answer haven't already been matched
					mask ^= 1 << i;
					(*(counts + c - 'A'))--;
					*(state + c - 'A' + rcant) |= 1 << i;
					(*(state + c - 'A' + rmin))++; // increment min count
				}
			}

			// Pass 2: remaining gray
			for (int i = 0; i < 5; ++i) {
				char c = *(guess + i);
				if (mask & 1 << i) { // all unmatched letters
					*(state + c - 'A' + rmax) = *(state + c - 'A' + rmin); // Set max to min
				}
			}

			free(counts);

			return state;
		}

		bool cmpadd ( char* dest, const char* word, const uint8_t* state ) {
			// check greens and yellows
			for (int i = 0; i < 26; ++i) {
				for (int j = 0; j < 5; ++j) {
					if (*(state + i + rmust) & (1 << j) && i != *(word + j) - 'A') return false; // green not match

					if (*(state + i + rcant) & (1 << j) && i == *(word + j) - 'A') return false; // yellow not match
				}
			}

			uint8_t* counts = countchars(word);

			for (int i = 0; i < 26; ++i) { // deal with min and max counts
				if (*(counts + i) < *(state + i + rmin) || *(counts + i) > *(state + i + rmax)) {
					free(counts);
					return false;
				}
			}

			strncpy(dest, word, 5);

			free(counts);
			return true;
		}

		wstate uwstate( const wstate& ows, const char* guess, const char* answer ) {
			ows.print();
			uint8_t* state = gstate(guess, answer);

			wstate ws;

			ws.valid = salloc(ows.lvalid * 5, sizeof(char));
			ws.solutions = salloc(ows.lsolutions * 5, sizeof(char));

			for (int i = 0; i < ows.lvalid; ++i) {
				ws.lvalid += cmpadd(ws.valid + ws.lvalid * 5, ows.valid + i * 5, state);
			}

			for (int i = 0; i < ows.lsolutions; ++i) {
				ws.lsolutions += cmpadd(ws.solutions + ws.lsolutions * 5, ows.solutions + i * 5, state);
			}

			// reallocate the memory with the new size
			// Safe because salloc initiates to null
			ws.valid = static_cast<char*> (realloc(ws.valid, (ws.lvalid * 5 + 1) * sizeof(char)));
			ws.solutions = static_cast<char*> (realloc(ws.solutions, (ws.lsolutions * 5 + 1) * sizeof(char)));

			ws.print();

			free(state);

			return ws;
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
			const int id; // output filename
			const char* gset; // guess set start
			const int l; // number of words allocated to this thread
			const wstate* state;
		};
		void* dfst ( void* arg ) {
			dfsta args = *((dfsta*) arg);

			printf("Starting thread %d. %d words allocated.\n", args.id, args.l);

			// char* fout;
			// sprintf(fout, "data/tmp/tmp-%d.dat", args.id);

			// FILE* out = fopen(fout, "w");

			const char* ans = "SWILL";

			wstate nws = uwstate(*args.state, "LOLLY", ans);

			puts(nws.valid);
			puts(nws.solutions);

			// if (fclose(out) != 0) {
			// 	printf("Failed to close tmp file %d after writing.", args.id);
			// }

			return nullptr;
		}
	}

	void init ( const char* fout, const char* fvalid, const char* fsolutions, const int threadcount ) {
		// Read in files to the char*
		rootstate.lsolutions = readfile(rootstate.solutions, fsolutions);
		rootstate.lvalid = readfile(rootstate.valid, fvalid);

		rootstate.lvalid += rootstate.lsolutions;

		// re allocate the string to copy over solutions as valid.
		rootstate.valid = static_cast<char*>(realloc(rootstate.valid, (rootstate.lvalid * 5 + 1) * sizeof(char)));

		strcat(rootstate.valid, rootstate.solutions);

		rootstate.print(true);

		// reset current guesses
		pos = 0;
		words = salloc(30, sizeof(char));
		strcpy(words, "                              "); // fill with spaces

		pthread_t* threads = static_cast<pthread_t*>(malloc(threadcount * sizeof(pthread_t)));
		int* trets = static_cast<int*>(malloc(threadcount * sizeof(int)));

		int ll = rootstate.lvalid;

		for (int i = 0; i < threadcount; ++i) {
			int alloc = ll / (threadcount - i);

			dfsta args = {i, rootstate.valid + (rootstate.lvalid - ll) * 5, alloc, &rootstate};

			ll -= alloc;

			*(trets + i) = pthread_create(threads + i, NULL, dfst, (void*) &args);
		}

		for (int i = 0; i < threadcount; ++i) {
			pthread_join(*(threads + i), NULL);

			printf("Thread %d returns with %d\n", i, *(trets + i));
		}
	}


	char* getstate () {
		char* p = salloc(30, sizeof(char));
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