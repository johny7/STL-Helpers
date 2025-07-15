
void LINQTest();
void lock_free_hash_map_tests();

int main()
{
	LINQTest();
	for(int i = 0; i < 1000000; ++i)
		lock_free_hash_map_tests();

	return 0;
}
