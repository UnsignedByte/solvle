/*
* @Author: UnsignedByte
* @Date:   2022-02-17 09:08:46
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-18 21:53:20
*/

#pragma once

namespace solver {

	char* getstate ();

	// initiate brute force solver and save all data to a file. 
	void init ( const char*, const char*, const char*, const int = 1 );

	void charEvent ( int );

}