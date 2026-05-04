#pragma once

class Random
{
private:
	int64_t seed;
	bool haveNextNextGaussian;
	double nextNextGaussian;
protected:
	int next(int bits);
public:
	Random();
	Random(int64_t seed);
	void setSeed(int64_t s);
	void nextBytes(byte *bytes, unsigned int count);
	double nextDouble();
	double nextGaussian();
	int nextInt();
	int nextInt(int to);
	float nextFloat();
	int64_t nextLong();
	bool nextBoolean();
	static int nextInt(Random* r, int n) { return r->nextInt(n); }
	static bool nextBoolean(Random* r) { return r->nextBoolean(); }
	static long long nextLong(Random* r) { return r->nextLong(); }
};