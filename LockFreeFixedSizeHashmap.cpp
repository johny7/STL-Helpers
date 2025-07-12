#include "LockFreeFixedSizeHashmap.h"

//	test1
//		thr1 - write/delete/write/delete same key
//		thr2 - successfully able to read the key most of the time

//	test2, a few buckets (bad hashing key function)
//		write one key
//		thr1 - writes and deletes lots of random stuff (but not the key)
//		thr2 - is able to clearly read the key

//	test3
//		thr1 - writes and deletes lot of random stuff
//		thr2 - reads non-existing key - always empty

//	test4
//		thr1 - writes set of keys, waits a bit and then shuffles and deletes all the keys
//		thr2 - keeps re-reading the same set of keys, amount should drop from N to 0
