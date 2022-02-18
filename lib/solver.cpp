/*
* @Author: UnsignedByte
* @Date:   2022-02-17 09:08:40
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-18 09:02:45
*/

#include <cstdio>
#include <iostream>
#include <cstring>

namespace solver {

	namespace {
		size_t pos = 0;
		char* words = nullptr;

		const char* fvalid = "data/valid.txt";
		const char* fsolutions = "data/solutions.txt";

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
	}

	char* getstate () {
		char* p = static_cast<char*>(malloc(30*sizeof(char)));
		return strcpy(p, words);
	}

	void init () {
		// Read in files to the char*
		lvalid = readfile(valid, fvalid);
		lsolutions = readfile(solutions, fsolutions);

		// reset current guesses
		pos = 0;
		words = static_cast<char*>(malloc(30*sizeof(char)));
		strcpy(words, "                              "); // fill with spaces
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

		puts(words);
	}

}