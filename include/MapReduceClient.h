#ifndef MAPREDUCECLIENT_H
#define MAPREDUCECLIENT_H

#include <vector>
#include <utility>

/**
 * Input key and value. The (key, value) for the map function and the MapReduceFramework.
 * The MapReduceFramework will call the map function with these pairs.
 */
class K1 {
public:
	virtual ~K1() = default;

	/**
	 * Compares this key with another key.
	 * Must be implemented by derived classes.
	 *
	 * @param other The other key to compare with this key.
	 * @return true if this key is less than the other key, false otherwise.
	 */
	virtual bool operator<(const K1 &other) const = 0;
};

class V1 {
public:
	virtual ~V1() = default;
};

/**
 * Intermediate key and value. The (key, value) for the Reduce function created by the Map function.
 * The MapReduceFramework will call the reduce function with these pairs.
 */
class K2 {
public:
	virtual ~K2() = default;

	/**
	 * Compares this key with another key.
	 * Must be implemented by derived classes.
	 *
	 * @param other The other key to compare with this key.
	 * @return true if this key is less than the other key, false otherwise.
	 */
	virtual bool operator<(const K2 &other) const = 0;
};

class V2 {
public:
	virtual ~V2() = default;
};

/**
 * Output key and value. The (key, value) for the Reduce function created by the Reduce function.
 */
class K3 {
public:
	virtual ~K3() = default;

	/**
	 * Compares this key with another key.
	 * Must be implemented by derived classes.
	 *
	 * @param other The other key to compare with this key.
	 * @return true if this key is less than the other key, false otherwise.
	 */
	virtual bool operator<(const K3 &other) const = 0;
};

class V3 {
public:
	virtual ~V3() = default;
};

/**
 * Defines the types of the input, intermediate, and output pairs used in the MapReduce framework.
 */
typedef std::pair<K1*, V1*> InputPair;
typedef std::pair<K2*, V2*> IntermediatePair;
typedef std::pair<K3*, V3*> OutputPair;

typedef std::vector<InputPair> InputVec;
typedef std::vector<IntermediatePair> IntermediateVec;
typedef std::vector<OutputPair> OutputVec;

/**
 * The MapReduceClient interface defines the methods that a client must implement
 * to perform map and reduce operations in the MapReduce framework.
 */
class MapReduceClient {
public:

	virtual  ~MapReduceClient() = default;

	/**
	 * Gets a single pair (K1, V1),
	 * and calls emit2(K2,V2, context) any number of times to output (K2, V2) pairs.
	 */
	virtual void map(const K1* key, const V1* value, void* context) const = 0;

	/**
	 * Gets a single K2 key and a vector of all its respective V2 values,
	 * and calls emit3(K3, V3, context) any number of times (usually once) to output (K3, V3) pairs.
	 */
	virtual void reduce(const IntermediateVec* pairs, void* context) const = 0;
};


#endif //MAPREDUCECLIENT_H
