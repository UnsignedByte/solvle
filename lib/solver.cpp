/*
* @Author: UnsignedByte
* @Date:   2022-02-17 09:08:40
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-19 23:26:14
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
		char* salloc (int len) {
			return static_cast<char*>(calloc(len + 1, sizeof(char)));
		}

		// Read a file of 5 character strings line by line, returns the number of files.
		size_t readfile (char*& str, const char* name) {
			FILE* f = fopen(name, "r");

			fseek(f, 0L, SEEK_END);
			long s = ftell(f);
			fseek(f, 0L, SEEK_SET);

			s = (s + 1) / 6 * 5; // Remove newlines from length

			str = salloc(s);

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
		uint8_t* countchars ( const char* word, uint8_t* charcounts ) {
			memset(charcounts, 0, 26);

			for (int j = 0; j < 5; ++j) {
				(*(charcounts + *(word + j) - 'A'))++; // increment count
			}

			return charcounts;
		}

		const int rmust = 0;
		const int rcant = 26;
		const int rmin = 26 * 2;
		const int rmax = 26 * 3;

		uint8_t* gstate( uint8_t* state, const char* guess, const char* answer ) {
			wstate ws;
			// 4 Parts:
			// 26 8 bit ints, low 5 bits of each represents whether the nth character must be in that position of the string
			// 26 8 bit ints, low 5 bits of each represents whether the nth character cant be in that position of the string
			// 26 8 bit ints, representing minimum count of each letter in ans
			// 26 8 bit ints, representing maximum count of each letter in ans
			memset(state, 0, 26 * 3);
			memset(state + rmax, 5, 26);

			uint8_t mask = 31;
			uint8_t* counts = state + 4 * 26;
			countchars(answer, counts);

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

			return state;
		}

		bool cmpadd ( char* dest, const char* word, uint8_t* state ) {
			// check greens and yellows
			for (int i = 0; i < 26; ++i) {
				for (int j = 0; j < 5; ++j) {
					if (*(state + i + rmust) & (1 << j) && i != *(word + j) - 'A') return false; // green not match

					if (*(state + i + rcant) & (1 << j) && i == *(word + j) - 'A') return false; // yellow not match
				}
			}

			uint8_t* counts = state + 4 * 26;
			countchars(word, counts);

			for (int i = 0; i < 26; ++i) { // deal with min and max counts
				if (*(counts + i) < *(state + i + rmin) || *(counts + i) > *(state + i + rmax)) {
					return false;
				}
			}

			strncpy(dest, word, 5);

			return true;
		}

		wstate uwstate( uint8_t* state, const wstate& ows, const char* guess, const char* answer ) {
			// ows.print();
			gstate(state, guess, answer);

			wstate ws;

			ws.valid = salloc(ows.lvalid * 5);
			ws.solutions = salloc(ows.lsolutions * 5);

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

			// ws.print();

			return ws;
		}

		uint64_t descend ( FILE* f, uint8_t* state, const wstate& ws, const char* ans, const int depth ) {
			if ( depth >= 6 ) {
				return 0;
			}

			if ( ws.lsolutions == 1 ) {
				return 0;
			}

			uint64_t sum = 0;

			for (int i = 0; i < ws.lvalid; ++i) {
				wstate nws = uwstate(state, ws, ws.valid + i * 5, ans);

				sum += descend(f, state, nws, ans, depth+1) + 1;

				if (depth == 0) {
					char* c = salloc(5);
					strncpy(c, ws.valid + i * 5, 5);
					printf("%d: Guessed %s, sum %lu\n", depth, c, sum);
					nws.print();
				}

			}

			return sum;
		}

/*
DATA FORMAT

<ANSWER>
<INITIAL VALID><INITIAL SOLS>
<GUESS>
<NUM REMAINING VALID><NUM SOLS>
<SUBGUESS 1>
<NUM REMAINING VALID><NUM SOLS>
...
<SUBGUESS N>
<NUM VALID>
...
<GUESS N>
<NUM VALID>
...
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

			const char* fmt = "data/tmp/tmp-%d.dat";

			size_t sz = snprintf(NULL, 0, fmt, args.id);
			char* fout = salloc(sz);
			snprintf(fout, sz + 1, fmt, args.id);

			FILE* out = fopen(fout, "w");

			uint8_t* state = static_cast<uint8_t*>(calloc(26 * 5, sizeof(uint8_t)));

			printf("%lu\n", descend(out, state, *args.state, "QUILL", 0));

			if (fclose(out) != 0) {
				printf("Failed to close tmp file %d after writing.", args.id);
			}

			free(state);

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

		// rootstate.print(true);

		// reset current guesses
		pos = 0;
		words = salloc(30);
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
		char* p = salloc(30);
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