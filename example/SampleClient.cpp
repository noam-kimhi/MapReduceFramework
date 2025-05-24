#include "../include/MapReduceFramework.h"
#include <cstdio>
#include <string>
#include <array>
#include <utility>
#include <unistd.h>

#define DELAY 150000
#define FIRST_STRING "This string is full of characters"
#define SECOND_STRING "Multithreading is awesome"
#define THIRD_STRING "race conditions are bad"
#define PRINT_STAGE(state) \
	printf("stage %d, %f%% \n", state.stage, state.percentage)
#define DONE "Done!\n"
#define PRINT_CHARACTER(c, count) \
	printf("The character %c appeared %d time%s\n", c, count, count > 1 ? "s" : "")

class VString final : public V1 {
public:

	explicit VString(std::string content) : content(std::move(content)) {}

	std::string content;
};

class KChar final : public K2, public K3 {
public:

	explicit KChar(const char c) : c(c) { }

	bool operator<(const K2 &other) const override {
		return c < dynamic_cast<const KChar&>(other).c;
	}

	bool operator<(const K3 &other) const override {
		return c < dynamic_cast<const KChar&>(other).c;
	}

	char c;
};

class VCount final : public V2, public V3{
public:

	explicit VCount(const int count) : count(count) {}

	int count;
};


class CounterClient final : public MapReduceClient {
public:

	void map(const K1* key, const V1* value, void* context) const override {
		std::array<int, 256> counts{};
		counts.fill(0);

		// Count the occurrences of each character in the string
		for(const char& c : dynamic_cast<const VString*>(value)->content) {
			counts[static_cast<unsigned char>(c)]++;
		}

		// Emit intermediate key-value pairs
		for (int i = 0; i < 256; ++i) {
			if (counts[i] == 0) {
				continue; // Skip characters that do not appear in the string
			}
			// Create a new key-value pair for each character with its count
			const auto k2 = new KChar(static_cast<signed char>(i));
			const auto v2 = new VCount(counts[i]);
			usleep(DELAY); // Simulate some processing delay, so prints are not too fast
			// Emit the key-value pair
			emit2(k2, v2, context);
		}
	}

	void reduce(const IntermediateVec* pairs, void* context) const override {
		const char c = dynamic_cast<const KChar*>(pairs->at(0).first)->c;
		int count = 0;
		// Sum the counts for each character
		for(const auto&[fst, snd]: *pairs) {
			count += dynamic_cast<const VCount*>(snd)->count;
			// After reduction, the pairs are no longer needed
			delete fst;
			delete snd;
		}
		// Create a new key-value pair for the character with its total count
		const auto k3 = new KChar(c);
		const auto v3 = new VCount(count);
		usleep(DELAY); // Simulate some processing delay, so prints are not too fast
		// Emit the final key-value pair
		emit3(k3, v3, context);
	}
};

int main(int argc, char** argv)
{
	const CounterClient client;
	InputVec inputVec;
	OutputVec outputVec;
	VString s1(FIRST_STRING);
	VString s2(SECOND_STRING);
	VString s3(THIRD_STRING);
	inputVec.emplace_back(nullptr, &s1);
	inputVec.emplace_back(nullptr, &s2);
	inputVec.emplace_back(nullptr, &s3);
	JobState state;
    JobState last_state = {UNDEFINED_STAGE, 0};
	JobHandle job = startMapReduceJob(client, inputVec, outputVec, 4);
	getJobState(job, &state);
    
	while (state.stage != REDUCE_STAGE || state.percentage != MAX_PERCENTAGE)
	{
        if (last_state.stage != state.stage || last_state.percentage != state.percentage){
        	PRINT_STAGE(state);
        }
		usleep(100000);
        last_state = state;
		getJobState(job, &state);
	}
	PRINT_STAGE(state);
	printf(DONE);
	
	closeJobHandle(job);
	
	for (auto&[fst, snd]: outputVec) {
		const char c = dynamic_cast<const KChar *>(fst)->c;
		const int count = dynamic_cast<const VCount *>(snd)->count;
		PRINT_CHARACTER(c, count);
		delete fst;
		delete snd;
	}
	
	return EXIT_SUCCESS;
}

