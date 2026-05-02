#include <iostream>
#include <limits>
#include <vector>
#include <cmath>
#include <memory>
#include <stdexcept>

using namespace std;

enum class UpdateType { ADD, SET, XOR, MUL, MIN, MAX, NONE };

/**
 * Represents a pending operation on a block.
 * Encapsulates the value and the type of operation to perform.
 */
template<typename T>
struct UpdateCommand {
    UpdateType type = UpdateType::NONE;
    T value = 0;

    // Helper to check if a command actually exists
    bool exists() const { return type != UpdateType::NONE; }
};

/**
 * @brief Abstract Policy interface for defining range query and update logic.
 * 
 * This interface follows the Monoid and Group Action mathematical structures.
 * The 'merge' defines the Monoid operation for queries, while 'apply' and 'compose'
 * define how updates (Actions) modify the underlying data.
 * 
 * @tparam T The underlying data type (e.g., int, long long).
 */
template <typename T>
class IPolicy {
public:
    virtual ~IPolicy() = default;

    /** @brief Returns the neutral element for the query operation (e.g., 0 for Sum, INF for Min). */
    virtual T identity() const = 0;

    /** @brief Combines two results to produce a block summary or query answer. */
    virtual T merge(const T& a, const T& b) const = 0;

    /** 
     * @brief Applies an update command to a value or a block summary.
     * @param current The current value (individual element or block summary).
     * @param cmd The update command to apply.
     * @param size The number of elements affected (1 for single elements, blockSize for blocks).
     */
    virtual T apply(const T& current, const UpdateCommand<T>& cmd, std::size_t size) const = 0;

    /** 
     * @brief Defines how to combine a new update command with an existing pending command (Lazy Composition).
     * @param oldCmd The existing pending command on the block.
     * @param newCmd The incoming command.
     */
    virtual UpdateCommand<T> compose(const UpdateCommand<T>& oldCmd, const UpdateCommand<T>& newCmd) const = 0;
};

template <typename T>
class SumUpdatePolicy : public IPolicy<T> {
public:
    T identity() const override { return 0; }

    T merge(const T& a, const T& b) const override { 
        return a + b; 
    }

    T apply(const T& current, const UpdateCommand<T>& cmd, std::size_t size) const override {
        switch (cmd.type) {
            case UpdateType::ADD: return current + (cmd.value * size);
            case UpdateType::SET: return cmd.value * size;
            default: return current;
        }
    }

    UpdateCommand<T> compose(const UpdateCommand<T>& old, const UpdateCommand<T>& newC) const override {
        if (newC.type == UpdateType::SET) return newC; // SET overwrites everything
        if (newC.type == UpdateType::ADD) {
            if (old.type == UpdateType::NONE) return newC;
            // Both ADD and SET can absorb an incoming ADD
            return {old.type, old.value + newC.value};
        }
        return old;
    }
};

template <typename T>
class MinUpdatePolicy : public IPolicy<T> {
public:
    // Identity for Min is Infinity
    T identity() const override { return std::numeric_limits<T>::max(); }

    T merge(const T& a, const T& b) const override { 
        return std::min(a, b); 
    }

    T apply(const T& current, const UpdateCommand<T>& cmd, std::size_t size) const override {
        switch (cmd.type) {
            // For RMQ, adding v to all elements increases the MIN by v (not v * size)
            case UpdateType::ADD: return (current == identity()) ? current : current + cmd.value;
            case UpdateType::SET: return cmd.value; 
            default: return current;
        }
    }
    
    UpdateCommand<T> compose(const UpdateCommand<T>& old, const UpdateCommand<T>& newC) const override {
        if (newC.type == UpdateType::SET) return newC; // SET overwrites everything
        if (newC.type == UpdateType::ADD) {
            if (old.type == UpdateType::NONE) return newC;
            // Both ADD and SET can absorb an incoming ADD
            return {old.type, old.value + newC.value};
        }
        return old;
    }
};


template<typename T>
class XORUpdatePolicy : public IPolicy<T>
{
    public:
        // Identity for Min is Infinity
        T identity() const override { return T(0);}

        T merge(const T& a, const T& b) const override { 
            return a ^ b; 
        }

        T apply(const T& current, const UpdateCommand<T>& cmd, std::size_t size) const override {
            switch (cmd.type) {
                case UpdateType::ADD:
                    throw std::runtime_error("ADD update is not supported for XOR-merge policies.");
                
                case UpdateType::SET: 
                    // If even number of elements, they all cancel out to 0.
                    // If odd, the XOR sum is the value itself.
                    return (size % 2 == 0) ? T(0) : cmd.value;
                
                case UpdateType::XOR:
                    // If size is even: (a^V ^ b^V) = a^b ^ (V^V) = current ^ 0.
                    // If size is odd: (a^V ^ b^V ^ c^V) = a^b^c ^ V = current ^ V.
                    return (size % 2 == 0) ? current : current ^ cmd.value;
                    
                default: return current;
            }
        }
        
        UpdateCommand<T> compose(const UpdateCommand<T>& old, const UpdateCommand<T>& newC) const override {
            // 1. If the new command is SET, it always wins and clears history
            if (newC.type == UpdateType::SET) {
                return newC;
            }

            // 2. If the block currently has no pending command, just take the new one
            if (old.type == UpdateType::NONE) {
                return newC;
            }

            // 3. Handle ADD composition
            if (newC.type == UpdateType::ADD) {
                // ADD + ADD = Sum of values
                // SET + ADD = Adjusted SET value (SET x, then ADD y = SET x+y)
                if (old.type == UpdateType::ADD || old.type == UpdateType::SET) {
                    return {old.type, old.value + newC.value};
                }
                // XOR + ADD is mathematically complex in O(1); 
                // Typically, you'd throw an exception here as discussed
                throw std::runtime_error("Composition of ADD on top of XOR is not supported in O(1).");
            }

            // 4. Handle XOR composition
            if (newC.type == UpdateType::XOR) {
                // XOR + XOR = Xor-sum of values
                // SET + XOR = Adjusted SET value (SET x, then XOR y = SET x^y)
                if (old.type == UpdateType::XOR || old.type == UpdateType::SET) {
                    return {old.type, old.value ^ newC.value};
                }
                // ADD + XOR is mathematically complex in O(1)
                throw std::runtime_error("Composition of XOR on top of ADD is not supported in O(1).");
            }

            return old;
        }
    
};


/**
 * @brief A generic Sqrt Decomposition data structure with Lazy Propagation.
 * 
 * Provides O(sqrt(N)) range updates and range queries by dividing the array into blocks.
 * Actual mathematical behavior is delegated to the provided IPolicy.
 * 
 * @tparam T The underlying data type stored in the structure.
 */
template<typename T>
class sqrtDecomp {
private:
    std::size_t numElems;                   ///< Total number of elements in the array.
    std::size_t blockSize;                  ///< Size of each square root block.
    std::size_t blockCount;                 ///< Total number of blocks.
    std::vector<T> arr;                     ///< The raw underlying array data.
    std::vector<T> blockArr;                ///< Precomputed summaries for each block.
    std::vector<UpdateCommand<T>> lazyArr;  ///< Pending update commands for each block.
    std::unique_ptr<IPolicy<T>> m_Policy;   ///< Strategy policy defining the math operations.

    /**
     * @brief Recomputes the summary (blockArr) for a specific block from its raw elements.
     * @param blockInd The index of the block to rebuild.
     */
    void rebuildBlock(std::size_t blockInd);

    /**
     * @brief Pushes a pending lazy update down to individual elements in a block.
     * @param blockInd The index of the block whose updates need to be applied.
     */
    void pushDownUpdate(std::size_t blockInd);

public:
    /**
     * @brief Constructs the Sqrt Decomposition structure.
     * @param inArr Initial data vector.
     * @param _pol A unique pointer to a concrete IPolicy implementation.
     */
    sqrtDecomp(const std::vector<T> &inArr, std::unique_ptr<IPolicy<T>> _pol);

    /**
     * @brief Performs a range update on indices [L, R].
     * @param L Starting index (inclusive).
     * @param R Ending index (inclusive).
     * @param type The type of update (ADD, SET, etc.).
     * @param val The value used for the update.
     * @throws std::out_of_range if L or R are invalid.
     */
    void update(std::size_t L, std::size_t R, UpdateType type, const T &val);

    /**
     * @brief Performs a range query on indices [L, R].
     * @param L Starting index (inclusive).
     * @param R Ending index (inclusive).
     * @return The aggregated result of the query based on the policy's merge logic.
     * @throws std::out_of_range if L or R are invalid.
     */
    T query(std::size_t L, std::size_t R);
};

template<typename T>
void sqrtDecomp<T>::rebuildBlock(std::size_t blockInd)
{
    std::size_t startInd = blockSize * blockInd;
    std::size_t endInd = std::min(numElems, startInd + blockSize);

    // Reset to identity so we don't double-count old values
    blockArr[blockInd] = m_Policy->identity();

    while(startInd < endInd)
        blockArr[blockInd] = m_Policy->merge(blockArr[blockInd], arr[startInd++]);
}

template<typename T>
void sqrtDecomp<T>::pushDownUpdate(std::size_t blockInd)
{
    if(lazyArr[blockInd].type != UpdateType::NONE)
    {
        std::size_t startInd = blockSize * blockInd;
        std::size_t endInd = std::min(numElems, startInd + blockSize);

        while(startInd < endInd)
        {
            arr[startInd] = m_Policy->apply(arr[startInd], lazyArr[blockInd], 1);
            startInd++;
        }

        lazyArr[blockInd] = {UpdateType::NONE, m_Policy->identity()};
    }
}

template<typename T>
sqrtDecomp<T>::sqrtDecomp(const std::vector<T> &inArr, std::unique_ptr<IPolicy<T>> _pol) :
    numElems(inArr.size()),
    m_Policy(std::move(_pol)),
    arr(inArr)
{
    blockSize = std::max<size_t>(1, static_cast<std::size_t>(std::sqrt(numElems)));
    blockCount = (numElems + blockSize -1) / blockSize;
    blockArr.assign(blockCount , m_Policy->identity());
    lazyArr.assign(blockCount, {UpdateType::NONE, m_Policy->identity()});

    // Now rebuild block array
    for(std::size_t i = 0; i < blockCount; i++)
        rebuildBlock(i);
}

template<typename T>
void sqrtDecomp<T>::update(std::size_t L, std::size_t R, UpdateType type, const T &val)
{

    if(L > numElems -1 || R > numElems - 1 || L > R)
        throw std::out_of_range("Index is out of bounds!");

    UpdateCommand<T> cmd{type, val};
    std::size_t bL = L / blockSize;
    std::size_t bR = R / blockSize;

    // It may be a full block but treat it as pertial block for simplicity
    if (bL == bR) {
        pushDownUpdate(bL);
        for (std::size_t i = L; i <= R; ++i) arr[i] = m_Policy->apply(arr[i], cmd, 1);
        rebuildBlock(bL);
    } else {
        // Left partial block
        pushDownUpdate(bL);
        std::size_t endL = (bL + 1) * blockSize;
        for (std::size_t i = L; i < endL; ++i) arr[i] = m_Policy->apply(arr[i], cmd, 1);
        rebuildBlock(bL);

        // Middle full blocks: Use the O(1) shortcuts!
        for (std::size_t b = bL + 1; b < bR; ++b) {
            // Determine how many elements are actually in this block
            std::size_t curStart = b * blockSize;
            std::size_t curEnd = std::min(numElems, curStart + blockSize);
            std::size_t actualSize = curEnd - curStart;

            // Use actualSize instead of the constant blockSize
            blockArr[b] = m_Policy->apply(blockArr[b], cmd, actualSize);
            lazyArr[b] = m_Policy->compose(lazyArr[b], cmd);
        }

        // It may not be the block to udpate at all if R is fully ocvered by last full block.
        // But this is done for code simplicity extra work here but pays off later.
        pushDownUpdate(bR);
        std::size_t startR = bR * blockSize;
        for (std::size_t i = startR; i <= R; ++i) arr[i] = m_Policy->apply(arr[i], cmd, 1);
        rebuildBlock(bR);
    }
}

template<typename T>
T sqrtDecomp<T>::query(std::size_t L, std::size_t R)
{
    if(L > numElems -1 || R > numElems - 1 || L > R)
        throw std::out_of_range("Index is out of bounds!");

    T res = m_Policy->identity();
    std::size_t bL = L / blockSize;
    std::size_t bR = R / blockSize;

    // It may be a full block but treat it as pertial block for simplicity
    if (bL == bR) {
        pushDownUpdate(bL);
        for (std::size_t i = L; i <= R; ++i) res = m_Policy->merge(res, arr[i]);
    } else {
        // Left partial block
        pushDownUpdate(bL);
        std::size_t endL = (bL + 1) * blockSize;
        for (std::size_t i = L; i < endL; ++i) res = m_Policy->merge(res, arr[i]);

        // Middle full blocks: Use the O(1) shortcuts!
        for (std::size_t b = bL + 1; b < bR; ++b) {
            res = m_Policy->merge(res, blockArr[b]);
        }

        // It may not be the block to udpate at all if R is fully ocvered by last full block.
        // But this is done for code simplicity extra work here but pays off later.
        pushDownUpdate(bR);
        std::size_t startR = bR * blockSize;
        for (std::size_t i = startR; i <= R; ++i) res = m_Policy->merge(res, arr[i]);
    }

    return res;
}


