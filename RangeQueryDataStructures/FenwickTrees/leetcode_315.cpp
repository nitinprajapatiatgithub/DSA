#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

using namespace std;

template<typename T>
class MergeStrategy
{
    public:
        virtual T merge(const T &a, const T &b) = 0;
        virtual T identity() = 0;
        virtual ~MergeStrategy() = default;
};

template<typename T, typename U>
class UpdateStrategy
{
    public:
        virtual T apply (const T &a, const U &b) = 0;
        virtual ~UpdateStrategy() = default;
};

template<typename T>
class InverseStrategy
{
    public:
        virtual T invert(const T &part, const T &total) = 0;
        virtual ~InverseStrategy() = default;
};

template<typename T>
class sumMergeStrategy : public MergeStrategy<T>
{
    public:
        virtual T merge(const T &a, const T &b) { return a + b;}
        virtual T identity() {return T();}
};

template<typename T, typename U>
class sumUpdateStrategy : public UpdateStrategy<T, U>
{
    public:
        virtual T apply(const T &a, const U &b){ return a + b;}
};

template<typename T>
class sumInverseStrategy : public InverseStrategy<T>
{
    public:
        // Inverse operation of sum is minus (-)
        virtual T invert(const T &part, const T&total) { return total - part;}
};

template<typename T, typename U>
class fenwickTree
{
    private:
        vector<T> tree_;
        unique_ptr<MergeStrategy<T>> mergeStrat_;
        unique_ptr<UpdateStrategy<T, U>> updateStrat_;
        unique_ptr<InverseStrategy<T>> inverseStrat_;
        long long treeSize_;

        void buildTree(const vector<T> &inArr);

    
    public:
        fenwickTree(const vector<T> &inArr, 
                    unique_ptr<MergeStrategy<T>> merge,
                    unique_ptr<UpdateStrategy<T, U>> updt,
                    unique_ptr<InverseStrategy<T>> invt)
                    : mergeStrat_(move(merge)),
                      updateStrat_(move(updt)),
                      inverseStrat_(move(invt))
        {
            treeSize_ = static_cast<long long>(inArr.size() + 1); 
            tree_.assign(treeSize_, mergeStrat_->identity());

            buildTree(inArr);
        }

        void update(long long ind, const U &val);
        T queryPrefix(long long ind);
        T rangeQuery(long long low, long long high);
};

template<typename T, typename U>
void fenwickTree<T, U>::buildTree(const vector<T> &inArr)
{
    for(long long i = 0; i < treeSize_ - 1; i++)
    {
        tree_[i + 1] = inArr[i];
    }

    for(long long i = 1; i < treeSize_; i++)
    {
        long long ind = i + (i & -i); // Immediate successor

        if(ind < treeSize_)
        {
            tree_[ind] = mergeStrat_->merge(tree_[ind],  tree_[i]);
        }
    }
}

template<typename T, typename U>
void fenwickTree<T, U>::update(long long ind, const U &val)
{

    // range check
    if(ind < 0 || ind > treeSize_ - 2)
    {
        throw std::out_of_range("Index is out of bounds!");
    }

    ind = ind + 1; // Remember fenwick tree starts from 1 index but user will give it from 0 
                   // as it gives w.r.t original array only
    
    while(ind < treeSize_)
    {
        tree_[ind] = updateStrat_->apply(tree_[ind], val);
        ind += (ind & -ind);
    }
}

template<typename T, typename U>
T fenwickTree<T, U>::queryPrefix(long long ind)
{
    // 1. Graceful Handling: Prefix of anything before the first element is the identity
    if (ind < 0) return mergeStrat_->identity();

    ind = ind + 1;

    // 2. Adjust Bounds Check: Only throw if we are truly beyond the array capacity
    if(ind >= treeSize_)
    {
        throw std::out_of_range("Index is out of bounds!");
    }

    T res = mergeStrat_->identity();

    while(ind > 0)
    {
        res = mergeStrat_->merge(res, tree_[ind]);
        ind -= (ind & -ind);
    }

    return res;
}

template<typename T, typename U>
T fenwickTree<T, U>::rangeQuery(long long low, long long high)
{
    // 1. Boundary Check: Only throw if indices are actually outside the array bounds
    if ((low < 0 || low > treeSize_ - 2) || (high < -1 || high > treeSize_ - 2))
    {
        throw std::out_of_range("Fenwick Tree: Index out of bounds!");
    }

    // 2. Functional Grace: If range is empty, return identity instead of crashing
    if (low > high) 
    {
        return mergeStrat_->identity();
    }
    
    if (!inverseStrat_) 
    {
        throw std::logic_error("Range query called on a non-invertible Fenwick Tree.");
    }
        
    T lowResult = queryPrefix(low - 1);
    T highResult = queryPrefix(high);

    return inverseStrat_->invert(lowResult, highResult);
}



class Solution {
public:
    vector<int> countSmaller(vector<int>& nums) {
        
        vector<int> sortedArr = nums;
        sort(sortedArr.begin(), sortedArr.end());
        sortedArr.erase(unique(sortedArr.begin(), sortedArr.end()), sortedArr.end());

        vector<int> freq(sortedArr.size(), 0);

        fenwickTree<int, int> tree(freq, 
                                    make_unique<sumMergeStrategy<int>>(),
                                    make_unique<sumUpdateStrategy<int, int>>(),
                                    make_unique<sumInverseStrategy<int>>());

        vector<int> result(nums.size(), 0);

        for(int i = nums.size() -1; i >=0; i--)
        {
            int rank = lower_bound(sortedArr.begin(), sortedArr.end(), nums[i]) - sortedArr.begin();

            result[i] += tree.queryPrefix(rank - 1);

            tree.update(rank, 1);
        }

        return result;
    }
};