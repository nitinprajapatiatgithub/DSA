#include <iostream>
#include <limits>
#include <vector>
#include <cmath>
#include <memory>
#include <stdexcept>

using namespace std;

/**
 * @brief Interface for merging operations in Range Query structures.
 * * Defines the mathematical properties (Binary Operation and Identity Element)
 * required for a Range Query data structure to aggregate data.
 * * @tparam T The underlying data type (int, long long, double, etc.).
 */
template<typename T>
class IMergeStrategy
{
    public:
        /**
         * @brief Performs the associative binary operation.
         * @param a Left operand.
         * @param b Right operand.
         * @return Result of the merge (e.g., a + b or min(a, b)).
         */
        virtual T merge(const T &a, const T &b) const = 0;

        /**
         * @brief Returns the identity element for the merge operation.
         * @return T(0) for sum, numeric_limits::max() for min, etc.
         */
        virtual T identity() const= 0;
        virtual ~IMergeStrategy() = default;
};

/**
 * @brief Contextual data container for point update events.
 * * Encapsulates the full state of a change to allow strategies to choose
 * between O(1) delta updates or O(sqrt N) rebuilds.
 */
template<typename T>
class updateEvent
{
    public:
        std::size_t index;
        std::size_t blockIndex;
        T oldVal;
        T newVal;

        updateEvent(std::size_t ind, std::size_t blkInd, const T &a, const T &b) :
                    index(ind),
                    blockIndex(blkInd),
                    oldVal(a),
                    newVal(b)
        {}
};

/**
 * @brief Interface for block update logic.
 * * Separates the "how" of updating a block summary from the data structure logic.
 */
template<typename T>
class IUpdateStrategy
{
    public:
        /**
         * @brief Updates a specific block summary after a point modification.
         * * @param blockArr Reference to the vector of block summaries.
         * @param origArr Reference to the underlying data array.
         * @param blockSize The size of each square root block.
         * @param event The context of the update (indices and values).
         * @param merger The merge strategy used for recalculation if needed.
         */
        virtual void update(std::vector<T> &blockArr,
                            std::vector<T> &origArr, 
                            std::size_t blockSize,
                            const updateEvent<T> &event,
                            const IMergeStrategy<T> &merger) const = 0;

        virtual ~IUpdateStrategy() = default;
};

template <typename T>
class sumMergeStrategy : public IMergeStrategy<T>
{
    public:
        T merge(const T &a , const T &b) const {return a + b;}
        T identity() const {return T(0);}
        ~sumMergeStrategy(){}
};

template<typename T>
class sumUpdateStrategy : public IUpdateStrategy<T>
{
    public:
        void update(std::vector<T> &blockArr,
                    std::vector<T> &origArr, 
                    std::size_t blockSize,
                    const updateEvent<T> &event,
                    const IMergeStrategy<T> &merger) const
        {
            blockArr[event.blockIndex] += (event.newVal - event.oldVal);
        }

        ~sumUpdateStrategy(){}
    
};

template<typename T>
class minMergeStrategy : public IMergeStrategy<T>
{
    public:
        // Assumption : std::min works on the T datatype
        T merge(const T &a , const T &b) const {return std::min(a, b); }
        T identity() const {return std::numeric_limits<T>::max();}
        ~minMergeStrategy(){}
};

template<typename T>
class minUpdateStrategy : public IUpdateStrategy<T>
{
    public:
        void update(std::vector<T> &blockArr,
                std::vector<T> &origArr, 
                std::size_t blockSize,
                const updateEvent<T> &event,
                const IMergeStrategy<T> &merger) const
        {
            std::size_t realStartInd = event.blockIndex * blockSize;
            std::size_t realEndInd = std::min(origArr.size(), realStartInd + blockSize);

            T res = merger.identity();

            while(realStartInd < realEndInd)
            {
                res = merger.merge(res, origArr[realStartInd]);
                realStartInd++;
            }

            blockArr[event.blockIndex] = res;
        }

        ~minUpdateStrategy(){}
};


/**
 * @brief A generic Square Root Decomposition framework for Range Queries and Point Updates.
 * * This class partitions an array into blocks of size approximately sqrt(N). 
 * It allows for pluggable strategies to handle different mathematical operations 
 * (Sum, Min, GCD, etc.) while maintaining O(sqrt N) query time.
 * * @tparam T The data type stored in the structure.
 */
template<typename T>
class sqrtDecomposition
{
    private:
        std::vector<T> arr;
        std::vector<T> blockArr;
        std::size_t blockSize;
        std::unique_ptr<IMergeStrategy<T>> _mergeStrat;
        std::unique_ptr<IUpdateStrategy<T>> _updateStrat;

    public:
        /**
         * @brief Constructs the decomposition structure.
         * * Performs an initial O(N) build by partitioning the input array and 
         * computing block summaries using the provided merge strategy.
         * * @param inArr Initial data vector.
         * @param mergeStrat Unique pointer to a merging policy (e.g., sumMergeStrategy).
         * @param updateStrat Unique pointer to an update policy (e.g., sumUpdateStrategy).
         * * @complexity O(N) time, O(N) space.
         */
        sqrtDecomposition(const std::vector<T> &inArr,
                          std::unique_ptr<IMergeStrategy<T>> mergeStrat,
                          std::unique_ptr<IUpdateStrategy<T>> updateStrat) :
                          arr(inArr),
                          _mergeStrat(std::move(mergeStrat)),
                          _updateStrat(std::move(updateStrat))
        {
            blockSize = std::max<std::size_t>(1, static_cast<std::size_t>((std::sqrt(inArr.size()))));
            blockArr.assign(((inArr.size() + blockSize -1)  / blockSize), _mergeStrat->identity());

            // fill up block
            for(std::size_t i = 0; i < inArr.size(); i++)
            {
                std::size_t blockIndex = i / blockSize;
                blockArr[blockIndex] = _mergeStrat->merge(blockArr[blockIndex], arr[i]);
            }
        }

        /**
         * @brief Updates a single element and its corresponding block summary.
         * * Updates the internal array and dispatches an updateEvent to the update strategy.
         * * @param index Zero-based index to update.
         * @param val New value to assign at the index.
         * @throw std::out_of_range if index >= array size.
         * * @complexity O(1) for reversible ops (Sum/XOR), O(sqrt N) for idempotent ops (Min/GCD).
         */
        void update(std::size_t index, const T &val)
        {
            if(index >= arr.size())
                throw std::out_of_range("Index is out of bounds!");

            updateEvent<T> event(index, index / blockSize, arr[index], val);

            arr[index] = val;

            _updateStrat->update(blockArr, arr, blockSize, event, *(_mergeStrat));
        }

        /**
         * @brief Queries the aggregated value in the range [L, R].
         * * Uses precomputed block summaries for middle blocks and iterates through 
         * individual elements for the "tails" at the boundaries.
         * * @param L Inclusive left boundary.
         * @param R Inclusive right boundary.
         * @return The merged result of all elements in [L, R]. Returns identity() if range is invalid.
         * * @complexity O(sqrt N)
         */
        T query(std::size_t L, std::size_t R)
        {
            T res = _mergeStrat->identity();
            
            if((L >= arr.size()) || (R >= arr.size()) || (L > R))
                return res;

            while(L <= R)
            {
                // full block overlap
                if( (L % blockSize == 0) && (R - L + 1 >= blockSize))
                {
                    res = _mergeStrat->merge(res, blockArr[L / blockSize]);
                    L += blockSize;        
                }
                // partial block overlap
                else
                {
                    std::size_t upperLim = (L / blockSize + 1) * blockSize;
                    std::size_t ind = L;
                    for(; ind < upperLim && ind <= R; ind++)
                        res = _mergeStrat->merge(res, arr[ind]);

                    L = ind;
                }
            }

            return res;
        }
};
