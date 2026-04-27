#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;

/**
 * @class mergeStrategy
 * @brief Abstract strategy interface for merging segment tree values.
 *
 * Defines the merge behavior used by the tree to combine two child values
 * and the identity element for the merge operation.
 *
 * @tparam T Type of values stored in the tree.
 */
template <typename T>
class mergeStrategy
{
    public:
        virtual T merge(const T &a, const T &b) = 0;
        virtual T identity() = 0;
        virtual ~mergeStrategy() {}
};

/**
 * @class updateStrategy
 * @brief Abstract strategy interface for updating segment tree values.
 *
 * Defines how an update value is applied to an existing node value and
 * provides an identity element for update combination.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class updateStrategy
{
    public:
        virtual T apply(const T &a, const U &b) = 0;
        virtual U identity() = 0;
        virtual ~updateStrategy() {}
};

/**
 * @class sumMergeStrategy
 * @brief Merge strategy for range sum operations.
 *
 * Implements sum behavior for segment tree nodes and returns 0 as the
 * identity element.
 *
 * @tparam T Type of values stored in the tree.
 */
template <typename T>
class sumMergeStrategy : public mergeStrategy<T>
{
    public:
        T merge(const T &a, const T &b) override
        {
            return a + b;
        }

        T identity() override
        {
            return T();
        }
};

/**
 * @class sumUpdateStrategy
 * @brief Update strategy for point add operations.
 *
 * Applies an additive update to a single tree value.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class sumUpdateStrategy : public updateStrategy<T, U>
{
    public:
        T apply(const T &a, const U &b) override
        {
            return a + static_cast<T>(b);
        }

        U identity() override
        {
            return U();
        }
};

/**
 * @class sumAssignStrategy
 * @brief Update strategy for point assignment operations.
 *
 * Replaces the existing value with the new update value.
 *
 * @tparam T Type of values stored in the tree.
 * @tparam U Type of update values.
 */
template <typename T, typename U>
class sumAssignStrategy : public updateStrategy<T, U>
{
    public:
        T apply(const T &a, const U &b) override
        {
            return static_cast<T>(b);
        }

        U identity() override
        {
            return U();
        }
};

template <typename T, typename U>
class segTree
{
    private:
        vector<T> tree_;
        unique_ptr<mergeStrategy<T>> mergeStrat_;
        unique_ptr<updateStrategy<T, U>> updateStrat_;
        int numElem;

        /**
         * @brief Recursively build the segment tree from input data.
         *
         * @param inArr Input array to build from
         */
         void build(const vector<T> &inArr);

        /**
         * @brief Recursively search for the merge result over a query range.
         *
         * Propagates tree values through partial overlaps and uses the merge
         * strategy to combine child results.
         *
         * @param low Current segment left index
         * @param high Current segment right index
         * @param i Query range left index
         * @param j Query range right index
         * @param pos Current node index in the tree array
         * @return Merged result for the query range inside this segment
         */
        T rangeSearch(int l , int r);

        /**
         * @brief Recursively update a single index value in the tree.
         *
         * Applies the update at the leaf and recomputes parent values using the
         * merge strategy while traversing back up the recursion.
         *
         * @param ind Index to update
         * @param val Update value to apply
         */
        void updateElem(int ind, const U &val);

    public:
        /**
         * @brief Construct a segment tree from an input array.
         *
         * @param inArr Input array used to initialize leaf values
         * @param mergeStrat Strategy object used to merge child nodes
         * @param updateStrat Strategy object used to apply update values
         */
        segTree(const vector<T> &inArr, unique_ptr<mergeStrategy<T>> mergeStrat, unique_ptr<updateStrategy<T, U>> updateStrat);

        /**
         * @brief Query the merge result over a half-open range [i, j].
         *
         * @param i Query range left index
         * @param j Query range right index
         * @return Merged result for the range
         * @throws std::out_of_range if query indices are invalid
         */
        T search(int i , int j);

        /**
         * @brief Update a single element in the segment tree.
         *
         * The update is applied at the leaf node, and all affected parents
         * are recomputed using the merge strategy.
         *
         * @param i Index to update
         * @param val Update value to apply at index i
         * @throws std::out_of_range if the index is invalid
         */
        void update(int i, const U &val);
};

template <typename T, typename U>
void segTree<T, U>::build(const vector<T> &inArr)
{
    // shift all elements to the right by numElem to make space for internal nodes
    for(int i = numElem - 1; i >= 0; --i)
    {
        tree_[numElem + i] = inArr[i];
    }

    // Now build the internal nodes by merging child values
    for(int i = numElem - 1; i > 0; --i)
    {
        tree_[i] = mergeStrat_->merge(tree_[i << 1], tree_[(i << 1) | 1]);
    }
}

template <typename T, typename U>
segTree<T, U>::segTree(const vector<T> &inArr, unique_ptr<mergeStrategy<T>> mergeStrat, unique_ptr<updateStrategy<T, U>> updateStrat):
         mergeStrat_(move(mergeStrat)),
         updateStrat_(move(updateStrat)),
         numElem(static_cast<int>(inArr.size()))
{
    if(numElem == 0)
        return;

    tree_.assign(2 * numElem, mergeStrat_->identity());

    // Now build the tree 
    build(inArr);   
}

template <typename T, typename U>
T segTree<T, U>::rangeSearch(int l , int r)
{

    /**
     * RANGE QUERY LOGIC: The "Buddy System" & Ancestor Climbing
     * * Iterative querying works bottom-up by traversing siblings (buddies).
     * A parent node represents the combined sum of its left (even index) 
     * and right (odd index) children.
     *
     * 1. Left Boundary (l):
     * - If 'l' is a Right Child (odd index): It is the "black sheep" of its 
     * buddy pair. Its left sibling is OUTSIDE the range. 
     * Action: Include 'l' in the sum, then move to the right (l++) to 
     * find the next potential buddy pair.
     * - If 'l' is a Left Child (even index): Both 'l' and its right buddy 
     * are potentially in range. 
     * Action: Don't take 'l' yet; move to the parent (l /= 2) to take 
     * the combined sum later.
     *
     * 2. Right Boundary (r) [Exclusive]:
     * - If 'r' is a Right Child (odd index): Since 'r' is exclusive, its 
     * Left Sibling (r-1) is the actual boundary.
     * - Action: Decrement 'r' (--r) and include that value. This sibling 
     * is now a "standalone" contributor because its buddy is 'r' (outside).
     * - If 'r' is a Left Child (even index): Both 'r' and its right sibling 
     * are outside the range.
     * Action: Move straight to the parent (r /= 2).
     *
     * 3. Convergence:
     * The loop continues until 'l' and 'r' meet, ensuring we've collected all 
     * partial fragments before they merge into a common ancestor.
     */

    T resL = mergeStrat_->identity();
    T resR = mergeStrat_->identity();

    while (l < r) 
    {
        if (l & 1) 
            resL = mergeStrat_->merge(resL, tree_[l++]);
        if (r & 1) 
            resR = mergeStrat_->merge(tree_[--r], resR);
        l >>= 1;
        r >>= 1;
    }

    return mergeStrat_->merge(resL, resR);

}

template <typename T, typename U>
T segTree<T, U>::search(int i , int j)
{
    if( (i < 0 || i > numElem -1) 
        || (j < 0 || j > numElem -1) 
        || (i > j))
    {
        throw std::out_of_range("Index is out of bounds!");
    }

    // Search in iterative tree work on extended range where left is inclusive and right is exclusive
    // [i, j] ---> [i + numElem, j + numElem + 1)

    return rangeSearch(i + numElem, j + numElem + 1);
}

template <typename T, typename U>
void segTree<T, U>::updateElem(int ind, const U &val)
{

    ind += numElem; // shift index to leaf position
    tree_[ind] = updateStrat_->apply(tree_[ind], val);

    while(ind > 1)
    {
        ind >>= 1; // move up to parent
        tree_[ind] = mergeStrat_->merge(tree_[ind << 1], tree_[(ind << 1) | 1]); // merge with sibling
    }
}

template <typename T, typename U>
void segTree<T, U>::update(int ind, const U &val)
{
    if( ind < 0 || ind > numElem - 1)
        throw std::out_of_range("Index is out of bounds!");

    updateElem(ind, val);
}

/* Leaving below code as commented as it provides usage guidance. */
/*
class NumArray {

    private:
        unique_ptr<segTree<int, int>> tree;

    public:

        NumArray(const vector<int>& nums) {
            tree = make_unique<segTree<int, int>>(nums,
                make_unique<sumMergeStrategy<int>>(),
                make_unique<sumUpdateStrategy<int, int>>());
        }
    
        void update(int index, int val) {
            tree->update(index, val);
        }
    
        int sumRange(int left, int right) {
            return tree->search(left, right);
        }
};
*/
/**
 * Your NumArray object will be instantiated and called as such:
 * NumArray* obj = new NumArray(nums);
 * obj->update(index,val);
 * int param_2 = obj->sumRange(left,right);
 */
